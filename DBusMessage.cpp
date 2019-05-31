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
    
    
    void DBusMessage::updatePadding(int32_t padding_size, std::vector<uint8_t>& buffer)
    {
        if (buffer.size() % padding_size)                                // is padding needed ?
        {
            int padding = padding_size - buffer.size() % padding_size;   // compute padding
            buffer.resize(buffer.size() + padding);                      // add padding
        }
    }
    
    
    void DBusMessage::insertBasic(DBUS_TYPE type, void const* data, std::vector<uint8_t>& buffer)
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
                if (type == DBUS_TYPE::SIGNATURE)
                {
                    uint8_t str_size = str->size();
                    insertBasic(DBUS_TYPE::BYTE, &str_size, buffer);
                }
                else
                {
                    uint32_t str_size = str->size();
                    insertBasic(DBUS_TYPE::UINT32, &str_size, buffer);
                }

                // Insert stirng + trailing null
                buffer.insert(buffer.end(), str->begin(), str->end());
                buffer.push_back('\0');
                return; // string inserted, do not insert the object directly.
            }
        }
        
        updatePadding(data_size, buffer);
        buffer.insert(buffer.end(), ptr, ptr + data_size);
    }
    
    
    void DBusMessage::insertVariant(DBUS_TYPE type, void const* data, uint32_t data_size)
    {
        body_.push_back(1);
        body_.push_back(static_cast<char>(type));
        body_.push_back('\0');
        
        insertBasic(data, data_size);
    }
    
    
    template<> 
    void DBusMessage::addArgument<int16_t>(int16_t const& arg)
    {
        signature_ += static_cast<char>(DBUS_TYPE::INT16);
        insertBasic(DBUS_TYPE::INT16, &arg, body_);
    }
    
    template<> 
    void DBusMessage::addArgument<uint16_t>(uint16_t const& arg)
    {
        signature_ += static_cast<char>(DBUS_TYPE::UINT16);
        insertBasic(DBUS_TYPE::UINT16, &arg, body_);
    }
    
    template<> 
    void DBusMessage::addArgument<int32_t>(int32_t const& arg)
    {
        signature_ += static_cast<char>(DBUS_TYPE::INT32);
        insertBasic(DBUS_TYPE::INT32, &arg, body_);
    }
    
    template<>
    void DBusMessage::addArgument<uint32_t>(uint32_t const& arg)
    {
        signature_ += static_cast<char>(DBUS_TYPE::UINT32);
        insertBasic(DBUS_TYPE::UINT32, &arg, body_);
    }
    
    template<> 
    void DBusMessage::addArgument<int64_t>(int64_t const& arg)
    {
        signature_ += static_cast<char>(DBUS_TYPE::INT64);
        insertBasic(DBUS_TYPE::INT64, &arg, body_);
    }
    
    template<> 
    void DBusMessage::addArgument<uint64_t>(uint64_t const& arg)
    {
        signature_ += static_cast<char>(DBUS_TYPE::UINT64);
        insertBasic(DBUS_TYPE::UINT64, &arg, body_);
    }
    
    template<>
    void DBusMessage::addArgument<bool>(bool const& arg)
    {
        signature_ += static_cast<char>(DBUS_TYPE::BOOLEAN);
        uint32_t dbus_bool = arg;
        insertBasic(DBUS_TYPE::BOOLEAN, &dbus_bool, body_);
    }
    
    template<> 
    void DBusMessage::addArgument<uint8_t>(uint8_t const& arg)
    {
        signature_ += static_cast<char>(DBUS_TYPE::BYTE);
        insertBasic(DBUS_TYPE::BYTE, &arg, body_);
    }
    
    
    template<>
    void DBusMessage::addArgument(std::string const& arg)
    {
        signature_ += static_cast<char>(DBUS_TYPE::STRING);
        insertBasic(DBUS_TYPE::STRING, &arg, body_);
    }
        
    
    template<>
    void DBusMessage::addArgument(ObjectPath const& arg)
    {
        signature_ += static_cast<char>(DBUS_TYPE::PATH);
        insertBasic(DBUS_TYPE::PATH, &arg.value, body_);
    }
        
    
    template<>
    void DBusMessage::addArgument(Signature const& arg)
    {
        signature_ += static_cast<char>(DBUS_TYPE::SIGNATURE);
        insertBasic(DBUS_TYPE::SIGNATURE, &arg.value, body_);
    }
    
    
    template<>
    void DBusMessage::addArgument(Variant const& arg)
    {
        auto insert = [](DBUS_TYPE type, void const* data, uint32_t data_size, std::vector<uint8_t>& buffer)
        {
            // Insert variant value signature.
            buffer.push_back(1);
            buffer.push_back(static_cast<char>(type));
            buffer.push_back('\0');
            
            // Insert value
            
        }
        
        signature_ += static_cast<char>(DBUS_TYPE::VARIANT);
        switch (arg.value.index())
        {
            case 0:
            {
                insertVariant(DBUS_TYPE::BYTE, &std::get<uint8_t>(arg.value), sizeof(uint8_t));
                break;                
            }
            case 1:
            {
                uint32_t dbus_bool = std::get<bool>(arg.value);
                insertVariant(DBUS_TYPE::BOOLEAN, &dbus_bool, sizeof(uint32_t));
                break;                
            }
            case 2:
            {
                insertVariant(DBUS_TYPE::INT16, &std::get<int16_t>(arg.value), sizeof(int16_t));
                break;                
            }
            case 3:
            {
                insertVariant(DBUS_TYPE::UINT16, &std::get<uint16_t>(arg.value), sizeof(uint16_t));
                break;                
            }
            case 4:
            {
                insertVariant(DBUS_TYPE::INT32, &std::get<int32_t>(arg.value), sizeof(int32_t));
                break;                
            }
            case 5:
            {
                insertVariant(DBUS_TYPE::UINT32, &std::get<uint32_t>(arg.value), sizeof(uint32_t));
                break;                
            }
            case 6:
            {
                insertVariant(DBUS_TYPE::INT64, &std::get<int64_t>(arg.value), sizeof(int64_t));
                break;                
            }
            case 7:
            {
                insertVariant(DBUS_TYPE::UINT64, &std::get<uint64_t>(arg.value), sizeof(uint64_t));
                break;                
            }
            case 8:
            {
                insertVariant(DBUS_TYPE::DOUBLE, &std::get<double>(arg.value), sizeof(double));
                break;                
            }
            case 9:
            {
                //string
                std::string const& str = std::get<std::string>(arg.value);
                uint32_t str_size = str.size();
                insertVariant(DBUS_TYPE::STRING, &str_size, sizeof(uint32_t)); // string start with a uint32_t for the size.
                body_.insert(body_.end(), str.begin(), str.end());
                body_.push_back('\0');
                break;
            }
            case 10:
            {    
                // path
                std::string const& str = std::get<ObjectPath>(arg.value).value;
                uint32_t str_size = str.size();
                insertVariant(DBUS_TYPE::PATH,  &str_size, sizeof(uint32_t)); // path ar string like type.
                body_.insert(body_.end(), str.begin(), str.end());
                body_.push_back('\0');
                break;
            }
        }
    }
}
