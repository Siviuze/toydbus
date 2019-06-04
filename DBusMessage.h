#ifndef DBUS_MESSAGE_H
#define DBUS_MESSAGE_H

#include <variant>
#include <vector>

#include "DBusError.h"

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
    
    
    class DBusConnection;
    class DBusMessage
    {
        friend class DBusConnection;
    public:
        DBusMessage()  = default;
        ~DBusMessage() = default;
        
        // return call serial;
        uint32_t prepareCall(std::string const& name, std::string const& path, std::string const& interface, std::string const& method);
        
        template<typename T> 
        void addArgument(T const& arg);
        
        template<typename T>
        DBusError extractArgument(T& arg);
        
        uint32_t serial() const { return header_.serial; }
        
    private:
        void serialize();
        void updatePadding(int32_t padding_size, std::vector<uint8_t>& buffer);
        void insertValue(DBUS_TYPE type, void const* data, std::vector<uint8_t>& buffer);
        
        static uint32_t serialCounter_;
        
        struct Header header_;
        HeaderFields fields_;
        Signature signature_;       // DBus call signature.
        
        std::vector<uint8_t> headerBuffer_;  // DBus message header buffer.
        std::vector<uint8_t> body_;          // DBus message body buffer.        
    };
}

#include "DBusMessage.tpp"

#endif // DBUS_MESSAGE_H
