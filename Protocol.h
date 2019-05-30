#ifndef DBUS_PROTOCOL_H
#define DBUS_PROTOCOL_H

#include <cstdint>
#include <string>
#include <variant>
#include <utility>

namespace dbus
{
    enum class DBUS_TYPE : uint8_t
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
        DICT_END      ='}'
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
        Signature(std::string const& signature) : value(signature) { };
        std::string value;
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
    typedef std::vector<std::pair<FIELD, std::variant<std::string, uint32_t, ObjectPath, Signature>>> HeaderFields;
}

#endif
