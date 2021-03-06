#ifndef DBUS_PROTOCOL_H
#define DBUS_PROTOCOL_H

#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

namespace dbus
{
    class DBusVariant;
    enum class DBUS_TYPE : char
    {
        ARRAY         = 'a',
        BOOLEAN       = 'b',
        BYTE          = 'y',
        DOUBLE        = 'd',
        INT16         = 'n',
        UINT16        = 'q',
        INT32         = 'i',
        UINT32        = 'u',
        INT64         = 'x',
        UINT64        = 't',
        PATH          = 'o',
        SIGNATURE     = 'g',
        STRING        = 's',
        UNIX_FD       = 'h',
        VARIANT       = 'v',
        STRUCT_BEGIN  = '(',
        STRUCT_END    = ')',
        DICT_BEGIN    = '{',
        DICT_END      = '}',
        UNKNOWN       = '~'
    };
    std::string str(DBUS_TYPE type);
    std::string prettyStr(DBUS_TYPE type);


    enum class MESSAGE_TYPE : uint8_t
    {
        INVALID       = 0,
        METHOD_CALL   = 1,
        METHOD_RETURN = 2,
        ERROR         = 3,
        SIGNAL        = 4
    };
    std::string str(MESSAGE_TYPE endianness);


    enum class ENDIANNESS : uint8_t
    {
        LITTLE ='l',
        BIG    ='B'
    };
    std::string str(ENDIANNESS endianness);


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
    std::string str(FIELD type);


    class ObjectPath
    {
    public:
        ObjectPath(std::string const& path = "")
            : data_(path)
        { }

        std::string const& data() const { return data_; }
        void setData(std::string const& data) { data_ = data; }
        void setData(std::string&& data) { data_ = std::move(data); }

    private:
        std::string data_;
    };
    bool operator==(ObjectPath const& lhs, ObjectPath const& rhs);
    bool operator<(ObjectPath const& lhs, ObjectPath const& rhs);
    std::ostream& operator<< (std::ostream& out, ObjectPath const& path);


    struct Signature : public std::string
    {
        Signature() = default;
        Signature(Signature const&) = default;

        using std::string::operator=;
        using std::string::operator+=;

        Signature& operator=(Signature const&) = default;
        Signature& operator+=(DBUS_TYPE type);
        bool operator==(DBUS_TYPE type);
        bool operator!=(DBUS_TYPE type);
    };


    template<typename K, typename V>
    using Dict = std::unordered_map<K, V>;


    // overload pattern.
    template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
    template<class... Ts> overload(Ts...) -> overload<Ts...>;

    // Get DBus type from C++ type.
    template<typename T>
    constexpr DBUS_TYPE dbusType()
    {
        if(std::is_same<T, FIELD>::value)                     { return DBUS_TYPE::BYTE;      }
        if(std::is_same<T, uint8_t>::value)                   { return DBUS_TYPE::BYTE;      }
        if(std::is_same<T, bool>::value)                      { return DBUS_TYPE::BOOLEAN;   }
        if(std::is_same<T, int16_t>::value)                   { return DBUS_TYPE::INT16;     }
        if(std::is_same<T, uint16_t>::value)                  { return DBUS_TYPE::UINT16;    }
        if(std::is_same<T, int32_t>::value)                   { return DBUS_TYPE::INT32;     }
        if(std::is_same<T, uint32_t>::value)                  { return DBUS_TYPE::UINT32;    }
        if(std::is_same<T, int64_t>::value)                   { return DBUS_TYPE::INT64;     }
        if(std::is_same<T, uint64_t>::value)                  { return DBUS_TYPE::UINT64;    }
        if(std::is_same<T, double>::value)                    { return DBUS_TYPE::DOUBLE;    }
        if(std::is_same<T, std::string>::value)               { return DBUS_TYPE::STRING;    }
        if(std::is_same<T, ObjectPath>::value)                { return DBUS_TYPE::PATH;      }
        if(std::is_same<T, Signature>::value)                 { return DBUS_TYPE::SIGNATURE; }
        if(std::is_same<T, DBusVariant>::value)               { return DBUS_TYPE::VARIANT;   }
        if(std::is_same<T, std::vector<DBusVariant>>::value)  { return DBUS_TYPE::ARRAY;     }

        return DBUS_TYPE::UNKNOWN;
    }


    struct Header
    {
        ENDIANNESS endianness{ENDIANNESS::LITTLE};
        MESSAGE_TYPE type;
        uint8_t flags{0};
        uint8_t version{1};
        uint32_t size;
        uint32_t serial{1};
    } __attribute__ ((packed));
    using HeaderFields = Dict<FIELD, DBusVariant>;
}

// Hash specializations
namespace std
{
    template<> struct hash<dbus::ObjectPath>
    {
        size_t operator()(const dbus::ObjectPath & path) const
        {
            return std::hash<std::string>{}(path.data());
        }
    };
}

#endif
