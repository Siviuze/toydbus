#ifndef DBUS_PROTOCOL_H
#define DBUS_PROTOCOL_H

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace dbus
{
    enum class DBUS_TYPE : char 
    {
        ARRAY         ='a',
        BOOLEAN       ='b',
        BYTE          ='y',
        DOUBLE        ='d',
        INT16         ='n',
        UINT16        ='q',
        INT32         ='i',
        UINT32        ='u',
        INT64         ='x',
        UINT64        ='t',
        PATH          ='o',
        SIGNATURE     ='g',
        STRING        ='s',
        UNIX_FD       ='h',
        VARIANT       ='v',
        STRUCT_BEGIN  ='(',
        STRUCT_END    =')',
        DICT_BEGIN    ='{',
        DICT_END      ='}',
        UNKNOWN
    };
    inline std::string str(DBUS_TYPE type)
    {
        std::string str(1, static_cast<char>(type));
        return str;
    }


    enum class MESSAGE_TYPE : uint8_t
    {
        INVALID       = 0,
        METHOD_CALL   = 1,
        METHOD_RETURN = 2,
        ERROR         = 3, 
        SIGNAL        = 4
    };


    enum class ENDIANNESS : uint8_t
    {
        LITTLE ='l',
        BIG    ='B'
    };


    struct ObjectPath
    {
        ObjectPath() = default;
        ObjectPath(std::string const& path) : value(path) { };
        std::string value;
    };


    struct Signature
    {
        //Signature(std::string const& signature) : value(signature) { };
        Signature() = default;
        Signature(DBUS_TYPE type) : value()
        {
            value += static_cast<char>(type);
        }
        
        Signature& operator+=(DBUS_TYPE type)
        {
            value += static_cast<char>(type);
            return *this;
        }
        
        bool operator==(DBUS_TYPE type)
        {
            return (*this == Signature(type));
        }
        
        bool operator!=(DBUS_TYPE type)
        {
            return not (*this == Signature(type));
        }
        
        bool operator==(Signature const& other)
        {
            return (value == other.value);
        }
        
        std::string value;
    };

    
    template<typename K, typename V>
    struct Dict
    {
        std::vector<std::pair<K, V>> value;
    };
    
    
    // overload pattern.
    template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
    template<class... Ts> overload(Ts...) -> overload<Ts...>;
    struct Variant
    {
        std::variant<uint8_t, bool, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, double, std::string, ObjectPath, Signature> value;
    };
    
    enum class FIELD : uint8_t
    {
        INVALID      = 0,
        PATH         = 1,
        INTERFACE    = 2,
        MEMBER       = 3,
        ERROR_NAME   = 4,
        REPLY_SERIAL = 5,
        DESTINATION  = 6,
        SENDER       = 7,
        SIGNATURE    = 8,
        UNIX_FDS     = 9
    };
    inline std::string str(FIELD type)
    {
        switch (type)
        {
            case FIELD::PATH:         { return "Path";         }
            case FIELD::INTERFACE:    { return "Interface";    }
            case FIELD::MEMBER:       { return "Member";       }
            case FIELD::ERROR_NAME:   { return "Error name";   }
            case FIELD::REPLY_SERIAL: { return "Reply serial"; }
            case FIELD::DESTINATION:  { return "Destination";  }
            case FIELD::SENDER:       { return "Sender";       }
            case FIELD::SIGNATURE:    { return "Signature";    }
            case FIELD::UNIX_FDS:     { return "UNIX FDS";     }
            default:
            case FIELD::INVALID:      { return "Invalid";      }
        }
    }
    
    
    struct Header
    {
        ENDIANNESS endianness{ENDIANNESS::LITTLE};
        MESSAGE_TYPE type;
        uint8_t flags{0};
        uint8_t version{1};
        uint32_t size;
        uint32_t serial{1};
        uint32_t fields_size;
    } __attribute__ ((packed));
    typedef Dict<FIELD, Variant> HeaderFields;
}

#endif
