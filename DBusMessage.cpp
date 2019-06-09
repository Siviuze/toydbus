#include <iostream> // debug

#include "helpers.h"
#include "DBusMessage.h"

namespace dbus
{
    // Init serial counter.
    uint32_t DBusMessage::serialCounter_ = 1U;
    
    uint32_t DBusMessage::prepareCall(const std::string& name, const std::string& path, const std::string& interface, const std::string& method)
    {
        header_ = {ENDIANNESS::LITTLE, MESSAGE_TYPE::METHOD_CALL, 0, 1, 0, serialCounter_};
        serialCounter_++;
      
        fields_ = 
        {{
            {FIELD::DESTINATION, {name}}, 
            {FIELD::PATH,        {ObjectPath{path}}}, 
            {FIELD::INTERFACE,   {interface}}, 
            {FIELD::MEMBER,      {method}},
        }};
        
        return serial();
    }
    
    
    std::string DBusMessage::dump() const
    {
        std::string dump;
        std::stringstream ss;
        
        ss << "----- Header -----" << std::endl;
        ss << "Endianess:   " << str(header_.endianness) << std::endl;
        ss << "Type:        " << str(header_.type) << std::endl;
        ss << "Flags:       " << (uint32_t)header_.flags << std::endl;
        ss << "Version:     " << (uint32_t)header_.version << std::endl;
        ss << "Size:        " << header_.size << std::endl;
        ss << "Serial:      " << header_.serial << std::endl;
        
        for (auto& f : fields_)
        {
            ss << str(f.first) << ": ";
            std::visit(overload
            {
                [&](auto const& arg)        { ss << arg; },
                [&](ObjectPath const& arg)  { ss << arg; },
                [&](Signature const& arg)   { ss << arg; },
            }, f.second);
            ss << std::endl;
        }
        ss << std::endl;
        
        ss << "---- header hex ----" << std::endl;
        ss << hexDump(headerBuffer_);
        
        ss << "----- Body hex -----" << std::endl;
        ss << hexDump(body_);
        
        return ss.str();
    }
    
    
    void DBusMessage::serialize()
    {
        if (not body_.empty())
        {
            // add signature to header fields.
            Variant v{signature_};
            fields_.emplace(FIELD::SIGNATURE, v);
        }
        
        // prepare buffer.
        headerBuffer_.clear();
        headerBuffer_.reserve(sizeof(struct Header) + header_.size + 256); // 256: preallocate memory for fields even if we dont know the finale size yet.
        
        // insert header
        header_.size = body_.size();  // Update header body size.
        uint8_t const* header_ptr = reinterpret_cast<uint8_t const*>(&header_); 
        headerBuffer_.insert(headerBuffer_.begin(), header_ptr, header_ptr+sizeof(struct Header));
        
        // insert fields
        uint8_t* const fields_size = &headerBuffer_.back() + 1; // WARNING: at this point, this pointer is out of bound!
        headerBuffer_.resize(headerBuffer_.size() + sizeof(uint32_t));
        for (auto& i : fields_)
        {
            updatePadding(8, headerBuffer_);                           // dict entry aligned on 8 bytes.
            insertValue(DBUS_TYPE::BYTE, &i.first, headerBuffer_);     // key.
            insertValue(DBUS_TYPE::VARIANT, &i.second, headerBuffer_); // value.
        }
        *reinterpret_cast<uint32_t*>(fields_size) = &headerBuffer_.back() - (fields_size + 3); // compute size
        updatePadding(8, headerBuffer_); // header size shall be a multiple of 8.
    }
    

    void DBusMessage::insertValue(DBUS_TYPE type, void const* data, std::vector<uint8_t>& buffer)
    {
        auto insertPOD = [this](void const* data, int32_t data_size, std::vector<uint8_t>& buffer)
        {
            updatePadding(data_size, buffer);
            
            uint8_t const* ptr = reinterpret_cast<uint8_t const*>(data);
            buffer.insert(buffer.end(), ptr, ptr + data_size);
        };
        
        switch (type)
        {
            case DBUS_TYPE::BYTE:
            {
                insertPOD(data, 1, buffer);
                break;
            }
            case DBUS_TYPE::INT16:
            case DBUS_TYPE::UINT16:
            {
                insertPOD(data, 2, buffer);
                break;
            }
            case DBUS_TYPE::BOOLEAN:
            case DBUS_TYPE::UINT32:
            case DBUS_TYPE::INT32:
            {
                insertPOD(data, 4, buffer);
                break;
            }
            case DBUS_TYPE::INT64:
            case DBUS_TYPE::UINT64:
            case DBUS_TYPE::DOUBLE:
            {
                insertPOD(data, 8, buffer);
                break;
            }
            case DBUS_TYPE::STRING:
            case DBUS_TYPE::PATH:
            case DBUS_TYPE::SIGNATURE:
            {
                std::string const* str = reinterpret_cast<std::string const*>(data);
                if (type == DBUS_TYPE::SIGNATURE) // signature have a size of one byte only.
                {
                    uint8_t str_size = str->size();
                    insertValue(DBUS_TYPE::BYTE, &str_size, buffer);
                }
                else
                {
                    uint32_t str_size = str->size();
                    insertValue(DBUS_TYPE::UINT32, &str_size, buffer);
                }

                // Insert string + trailing null
                buffer.insert(buffer.end(), str->begin(), str->end());
                buffer.push_back('\0');
                break;
            }
            case DBUS_TYPE::VARIANT:
            {
                auto insertVariant = [this](DBUS_TYPE type, void const* data, std::vector<uint8_t>& buffer)
                {
                    Signature s{str(type)};
                    insertValue(DBUS_TYPE::SIGNATURE, &s, buffer);
                    insertValue(type, data, buffer);
                };
                
                Variant const& v = *reinterpret_cast<Variant const*>(data);
                std::visit(overload
                {
                    [&](auto arg)               { std::abort(); }, // should'nt be possible
                    [&](uint8_t arg)            { insertVariant(DBUS_TYPE::BYTE,      &arg, buffer); },
                    [&](bool arg)               { insertVariant(DBUS_TYPE::BOOLEAN,   &arg, buffer); },
                    [&](int16_t arg)            { insertVariant(DBUS_TYPE::INT16,     &arg, buffer); },
                    [&](uint16_t arg)           { insertVariant(DBUS_TYPE::UINT16,    &arg, buffer); },
                    [&](int32_t arg)            { insertVariant(DBUS_TYPE::INT32,     &arg, buffer); },
                    [&](uint32_t arg)           { insertVariant(DBUS_TYPE::UINT32,    &arg, buffer); },
                    [&](int64_t arg)            { insertVariant(DBUS_TYPE::INT64,     &arg, buffer); },
                    [&](uint64_t arg)           { insertVariant(DBUS_TYPE::UINT64,    &arg, buffer); },
                    [&](double arg)             { insertVariant(DBUS_TYPE::DOUBLE,    &arg, buffer); },
                    [&](std::string const& arg) { insertVariant(DBUS_TYPE::STRING,    &arg, buffer); },
                    [&](ObjectPath const& arg)  { insertVariant(DBUS_TYPE::PATH,      &arg, buffer); },
                    [&](Signature const& arg)   { insertVariant(DBUS_TYPE::SIGNATURE, &arg, buffer); },
                }, v);
                
                break;
            }
            case DBUS_TYPE::ARRAY:
            case DBUS_TYPE::UNIX_FD:
            case DBUS_TYPE::STRUCT_BEGIN:
            case DBUS_TYPE::STRUCT_END:
            case DBUS_TYPE::DICT_BEGIN:
            case DBUS_TYPE::DICT_END:
            case DBUS_TYPE::UNKNOWN:
            {
                std::abort();
            }
        }
    }
    
    
    DBusError DBusMessage::checkSignature(DBUS_TYPE type)
    {
        DBUS_TYPE next_entry_type = static_cast<DBUS_TYPE>(signature_[sign_pos_]);
        if (next_entry_type != type)
        {
            return EERROR("Wrong signature: expected '" + str(type) + "', got '" + str(next_entry_type) + "'");
        }
        sign_pos_++;
        
        return ESUCCESS;
    }
    
    
    DBusError DBusMessage::extractArgument(DBUS_TYPE type, void* data)
    {
        switch (type)
        {
            case DBUS_TYPE::BYTE: 
            {
                uint8_t* arg = reinterpret_cast<uint8_t*>(data);
                *arg = *reinterpret_cast<uint8_t*>(body_.data() + body_pos_);
                body_pos_ += sizeof(uint8_t);
                break;
            }
            case DBUS_TYPE::BOOLEAN: 
            {
                bool* arg = reinterpret_cast<bool*>(data);
                uint32_t dbus_bool;
                extractArgument(DBUS_TYPE::UINT32, &dbus_bool);
                *arg = dbus_bool;
                break;
            }
            case DBUS_TYPE::INT16: 
            case DBUS_TYPE::UINT16:
            {
                align(body_pos_, sizeof(uint16_t));
                uint16_t* arg = reinterpret_cast<uint16_t*>(data);
                *arg = *reinterpret_cast<uint16_t*>(body_.data() + body_pos_);
                body_pos_ += sizeof(uint16_t);
                break;
            }
            case DBUS_TYPE::INT32:
            case DBUS_TYPE::UINT32:
            {
                align(body_pos_, sizeof(uint32_t));
                uint32_t* arg = reinterpret_cast<uint32_t*>(data);
                *arg = *reinterpret_cast<uint32_t*>(body_.data() + body_pos_);
                body_pos_ += sizeof(uint32_t);
                break;
            }
            case DBUS_TYPE::INT64:
            case DBUS_TYPE::UINT64:
            {
                align(body_pos_, sizeof(uint64_t));
                uint64_t* arg = reinterpret_cast<uint64_t*>(data);
                *arg = *reinterpret_cast<uint64_t*>(body_.data() + body_pos_);
                body_pos_ += sizeof(uint64_t);
                break;
            }
            case DBUS_TYPE::STRING:
            case DBUS_TYPE::PATH: 
            case DBUS_TYPE::SIGNATURE:
            {
                uint32_t str_size;
                if (type == DBUS_TYPE::SIGNATURE)
                {
                    uint8_t sign_size;
                    extractArgument(DBUS_TYPE::BYTE, &sign_size);
                    str_size = sign_size;
                }
                else
                {
                    extractArgument(DBUS_TYPE::UINT32, &str_size);
                }
                
                std::string* arg = reinterpret_cast<std::string*>(data);
                arg->insert(arg->begin(), body_.data() + body_pos_, body_.data() + body_pos_ + str_size);
                body_pos_ += str_size + 1; // trailing nul.
                break;
            }
            case DBUS_TYPE::VARIANT: 
            {
                Variant* arg = reinterpret_cast<Variant*>(data);
                Signature s;
                extractArgument(DBUS_TYPE::SIGNATURE, &s);
                if (s.size() != 1)
                {
                    return EERROR("Variant signature length shall be 1, got: " + std::to_string(s.size()));
                }
                
                DBUS_TYPE variant_type = static_cast<DBUS_TYPE>(s[0]);
                
                DBusError err;
                switch (variant_type)
                {
                    case DBUS_TYPE::BYTE:    
                    { 
                        uint8_t val; 
                        err = extractArgument(variant_type, &val);
                        *arg = val;
                        break;
                    }
                    case DBUS_TYPE::BOOLEAN: 
                    {
                        bool val;
                        err = extractArgument(variant_type, &val);
                        *arg = val;
                        break; 
                    }
                    case DBUS_TYPE::INT16:   
                    { 
                        int16_t val;
                        err = extractArgument(variant_type, &val);
                        *arg = val;
                        break;
                    }
                    case DBUS_TYPE::UINT16:  
                    { 
                        uint16_t val;
                        err = extractArgument(variant_type, &val);
                        *arg = val;
                        break; 
                    }
                    case DBUS_TYPE::INT32:   
                    { 
                        int32_t val;
                        err = extractArgument(variant_type, &val);
                        *arg = val;
                        break;
                    }
                    case DBUS_TYPE::UINT32:  
                    { 
                        uint32_t val;
                        err = extractArgument(variant_type, &val);
                        *arg = val;
                        break;
                    }
                    case DBUS_TYPE::INT64:   
                    { 
                        int64_t val;
                        err = extractArgument(variant_type, &val);
                        *arg = val;
                        break;
                    }
                    case DBUS_TYPE::UINT64:  
                    { 
                        uint64_t val;
                        err = extractArgument(variant_type, &val);
                        *arg = val;
                        break;
                    }
                    case DBUS_TYPE::STRING:  
                    { 
                        std::string val;
                        err = extractArgument(variant_type, &val);
                        *arg = val;
                        break; 
                    }
                    case DBUS_TYPE::SIGNATURE:
                    {
                        Signature val;
                        err = extractArgument(variant_type, &val);
                        *arg = val;
                        break;
                    }
                    default: 
                    { 
                        return EERROR("Variant: unsupported type " + str(type)); 
                    }
                }
                
                return err;
            }
            default:
            {
                return EERROR("Unsupported type " + str(type));
            }
        }
        
        return ESUCCESS;
    }
}
