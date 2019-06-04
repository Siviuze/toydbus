/*
 * Copyright (c) 2019, leduc <philippe.leduc@mailfence.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY leduc <philippe.leduc@mailfence.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL leduc <philippe.leduc@mailfence.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>

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
        uint8_t const* ptr = reinterpret_cast<uint8_t const*>(data);
        uint32_t data_size = 0;
        switch (type)
        {
            case DBUS_TYPE::BYTE:
            {
                data_size = 1;
                break;
            }
            case DBUS_TYPE::INT16:
            case DBUS_TYPE::UINT16:
            {
                data_size = 2;
                break;
            }
            case DBUS_TYPE::BOOLEAN:
            case DBUS_TYPE::UINT32:
            case DBUS_TYPE::INT32:
            {
                data_size = 4;
                break;
            }
            case DBUS_TYPE::INT64:
            case DBUS_TYPE::UINT64:
            case DBUS_TYPE::DOUBLE:
            {
                data_size = 8;
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
        }
        
        updatePadding(data_size, buffer);
        buffer.insert(buffer.end(), ptr, ptr + data_size);
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
    void DBusMessage::addArgument(Dict<FIELD, Variant> const& arg)
    {
        /*
        signature_ += static_cast<char>(DBUS_TYPE::ARRAY);
        signature_ += static_cast<char>(DBUS_TYPE::DICT_BEGIN);
        signature_ += static_cast<char>(DBUS_TYPE::BYTE);
        signature_ += static_cast<char>(DBUS_TYPE::VARIANT);
        signature_ += static_cast<char>(DBUS_TYPE::DICT_END);
        */ // No signature since it is specific to the header
        
        for (auto& i : arg.value)
        {
            updatePadding(8, body_); // dict entry aligned on 8 bytes.
            
            // insert key
            insertValue(DBUS_TYPE::BYTE, &i.first, body_);
            
            // insert value
            insertValue(DBUS_TYPE::VARIANT, &i.second, body_);
        }
    }
}
