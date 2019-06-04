#ifndef DBUS_MESSAGE_H
#define DBUS_MESSAGE_H

#include "Protocol.h"
#include "DBusError.h"

namespace dbus
{
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
        std::string dump() const;
        
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
        
        uint32_t signPos_{0};
        uint32_t bodyPos_{0};
    };
}

#include "DBusMessage.tpp"

#endif // DBUS_MESSAGE_H
