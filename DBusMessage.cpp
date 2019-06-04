#include <sstream>
#include <iomanip>

#include <iostream> // debug

#include "DBusMessage.h"

namespace dbus
{
    // Init serial counter.
    uint32_t DBusMessage::serialCounter_ = 1U;
    
    uint32_t DBusMessage::prepareCall(const std::string& name, const std::string& path, const std::string& interface, const std::string& method)
    {
        header_ = {ENDIANNESS::LITTLE, MESSAGE_TYPE::METHOD_CALL, 0, 1, 0, serialCounter_, 0};
        serialCounter_++;
      
        fields_ = 
        {{
            {FIELD::DESTINATION, {name}}, 
            {FIELD::PATH,        {ObjectPath(path)}}, 
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
        ss << "Endianess: " << static_cast<char>(header_.endianness) << std::endl;
        ss << "Type: ";
        switch (header_.type)
        {
            case MESSAGE_TYPE::METHOD_CALL:   { ss << "method_call";   break; }
            case MESSAGE_TYPE::ERROR:         { ss << "error";         break; }
            case MESSAGE_TYPE::METHOD_RETURN: { ss << "method_return"; break; }
            case MESSAGE_TYPE::SIGNAL:        { ss << "signal";        break; }
            case MESSAGE_TYPE::INVALID:
            default:
            {
                ss << "invalid";
            }
        }
        ss << std::endl;
        ss << "Flags:       " << (uint32_t)header_.flags << std::endl;
        ss << "Version:     " << (uint32_t)header_.version << std::endl;
        ss << "Size:        " << header_.size << std::endl;
        ss << "Serial:      " << header_.serial << std::endl;
        ss << "Fields size: " << header_.fields_size << std::endl;
        
        for (auto& f : fields_.value)
        {
            ss << str(f.first) << ": ";
            std::visit(overload
            {
                [&](auto const& arg)        { ss << arg; },
                [&](ObjectPath const& arg)  { ss << arg.value; },
                [&](Signature const& arg)   { ss << arg.value; },
            }, f.second.value);
            ss << std::endl;
        }
        
        ss << "----- Body -----" << std::endl;
        // old school dump 
        ss << std::hex << std::setfill('0');
        for (auto i : body_)
        {
            ss << (uint32_t)i; //static_cast<int32_t>(i);
        }
        
        return ss.str();
    }
    
    
    void DBusMessage::serialize()
    {
        if (not body_.empty())
        {
            // add signature to header fields.
            Variant v;
            v.value = signature_;
            fields_.value.emplace_back(FIELD::SIGNATURE, v);
        }
        
        std::vector<uint8_t> fields;
        for (auto& i : fields_.value)
        {
            updatePadding(8, fields);                           // dict entry aligned on 8 bytes.
            insertValue(DBUS_TYPE::BYTE, &i.first, fields);     // key.
            insertValue(DBUS_TYPE::VARIANT, &i.second, fields); // value.
        }
        
        // Update header with sizes.
        header_.size = body_.size();
        header_.fields_size = fields.size();
        
        // serialize all data
        headerBuffer_.clear();
        headerBuffer_.reserve(sizeof(struct Header) + header_.size + header_.fields_size);
        
        // insert header
        uint8_t const* header_ptr = reinterpret_cast<uint8_t const*>(&header_); 
        headerBuffer_.insert(headerBuffer_.begin(), header_ptr, header_ptr+sizeof(struct Header));
        headerBuffer_.insert(headerBuffer_.end(), fields.begin(), fields.end());
        updatePadding(8, headerBuffer_); // header size shall be a multiple of 8.
    }
    
    
    void DBusMessage::updatePadding(int32_t padding_size, std::vector<uint8_t>& buffer)
    {
        if (buffer.size() % padding_size)                                  // is padding needed ?
        {
            int32_t padding = padding_size - buffer.size() % padding_size; // compute padding
            buffer.resize(buffer.size() + padding);                        // add padding
        }
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
                return; // string inserted, do not insert the object directly.
            }
            case DBUS_TYPE::VARIANT:
            {
                auto insertVariant = [this](DBUS_TYPE type, void const* data, std::vector<uint8_t>& buffer)
                {
                    Signature s(type);
                    insertValue(DBUS_TYPE::SIGNATURE, &s, buffer);
                    insertValue(type, data, buffer);
                };
                
                Variant const* v = reinterpret_cast<Variant const*>(data);
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
                    [&](ObjectPath const& arg)  { insertVariant(DBUS_TYPE::PATH,      &arg.value, buffer); },
                    [&](Signature const& arg)   { insertVariant(DBUS_TYPE::SIGNATURE, &arg.value, buffer); },
                }, v->value);
                
                return; // variant inserted, do not insert the object directly.
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

    
    template<> 
    void DBusMessage::addArgument<int16_t>(int16_t const& arg)
    {
        signature_ += DBUS_TYPE::INT16;
        insertValue(DBUS_TYPE::INT16, &arg, body_);
    }
    
    template<> 
    void DBusMessage::addArgument<uint16_t>(uint16_t const& arg)
    {
        signature_ += DBUS_TYPE::UINT16;
        insertValue(DBUS_TYPE::UINT16, &arg, body_);
    }
    
    template<> 
    void DBusMessage::addArgument<int32_t>(int32_t const& arg)
    {
        signature_ += DBUS_TYPE::INT32;
        insertValue(DBUS_TYPE::INT32, &arg, body_);
    }
    
    template<>
    void DBusMessage::addArgument<uint32_t>(uint32_t const& arg)
    {
        signature_ += DBUS_TYPE::UINT32;
        insertValue(DBUS_TYPE::UINT32, &arg, body_);
    }
    
    template<> 
    void DBusMessage::addArgument<int64_t>(int64_t const& arg)
    {
        signature_ += DBUS_TYPE::INT64;
        insertValue(DBUS_TYPE::INT64, &arg, body_);
    }
    
    template<> 
    void DBusMessage::addArgument<uint64_t>(uint64_t const& arg)
    {
        signature_ += DBUS_TYPE::UINT64;
        insertValue(DBUS_TYPE::UINT64, &arg, body_);
    }
    
    template<>
    void DBusMessage::addArgument<bool>(bool const& arg)
    {
        signature_ += DBUS_TYPE::BOOLEAN;
        uint32_t dbus_bool = arg;
        insertValue(DBUS_TYPE::BOOLEAN, &dbus_bool, body_);
    }
    
    template<> 
    void DBusMessage::addArgument<uint8_t>(uint8_t const& arg)
    {
        signature_ += DBUS_TYPE::BYTE;
        insertValue(DBUS_TYPE::BYTE, &arg, body_);
    }
    
    
    template<>
    void DBusMessage::addArgument(std::string const& arg)
    {
        signature_ += DBUS_TYPE::STRING;
        insertValue(DBUS_TYPE::STRING, &arg, body_);
    }
        
    
    template<>
    void DBusMessage::addArgument(ObjectPath const& arg)
    {
        signature_ += DBUS_TYPE::PATH;
        insertValue(DBUS_TYPE::PATH, &arg.value, body_);
    }
        
    
    template<>
    void DBusMessage::addArgument(Signature const& arg)
    {
        signature_ += DBUS_TYPE::SIGNATURE;
        insertValue(DBUS_TYPE::SIGNATURE, &arg.value, body_);
    }
    
    
    template<>
    void DBusMessage::addArgument(Variant const& arg)
    {
        signature_ += DBUS_TYPE::VARIANT;
        insertValue(DBUS_TYPE::VARIANT, &arg, body_);
    }
    
    template<>
    DBusError DBusMessage::extractArgument(std::string& arg)
    {
        DBUS_TYPE next_entry_type = static_cast<DBUS_TYPE>(signature_.value[signPos_]);
        if (next_entry_type != DBUS_TYPE::STRING)
        {
            return EERROR("Wrong signature: expected 's', got '" + str(next_entry_type) + "'");
        }
        
        // string aligned on 4 bytes
        if (bodyPos_ % 4)
        {
            bodyPos_ += 4 - (bodyPos_ % 4);
        }
        
        uint32_t str_size = *reinterpret_cast<uint32_t*>(body_.data() + bodyPos_);
        bodyPos_ += 4;
        
        arg.clear();
        arg.insert(arg.begin(), body_.data() + bodyPos_, body_.data() + bodyPos_ + str_size);
        bodyPos_ += str_size;
        
        return ESUCCESS;
    }
    
    /*
    template<> 
    void DBusMessage::addArgument(Dict<FIELD, Variant> const& arg)
    {
        signature_ += static_cast<char>(DBUS_TYPE::ARRAY);
        signature_ += static_cast<char>(DBUS_TYPE::DICT_BEGIN);
        signature_ += static_cast<char>(DBUS_TYPE::BYTE);
        signature_ += static_cast<char>(DBUS_TYPE::VARIANT);
        signature_ += static_cast<char>(DBUS_TYPE::DICT_END);
        
        for (auto& i : arg.value)
        {
            updatePadding(8, body_); // dict entry aligned on 8 bytes.
            
            // insert key
            insertValue(DBUS_TYPE::BYTE, &i.first, body_);
            
            // insert value
            insertValue(DBUS_TYPE::VARIANT, &i.second, body_);
        }
    }
    */
}
