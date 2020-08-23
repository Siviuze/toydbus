#ifndef DBUS_MESSAGE_H
#define DBUS_MESSAGE_H

#include "Protocol.h"
#include "DBusError.h"
#include "helpers.h"

namespace dbus
{
    class DBusConnection;
    class DBusVariant;
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

        template<typename K, typename V>
        void addArgument(Dict<K, V> const& arg);

        template<typename T>
        DBusError extractArgument(T& arg);

        template<typename K, typename V>
        DBusError extractArgument(Dict<K, V>& arg);

        uint32_t serial() const { return header_.serial; }
        std::string dump() const;

        // helpers to handle message field.
        MESSAGE_TYPE type() const { return header_.type; }
        bool isReply() const      { return header_.type == MESSAGE_TYPE::METHOD_RETURN; }
        bool isError() const      { return header_.type == MESSAGE_TYPE::ERROR;         }
        bool isSignal() const     { return header_.type == MESSAGE_TYPE::SIGNAL;        }

        // Optionnal header fields accessors
        uint32_t replySerial() const            { return std::get<uint32_t>(fields_.at(FIELD::REPLY_SERIAL));  }
        std::string const& errorMessage() const { return std::get<std::string>(fields_.at(FIELD::ERROR_NAME)); }
        ObjectPath const& path() const          { return std::get<ObjectPath>(fields_.at(FIELD::PATH));        }
        std::string const& interface() const    { return std::get<std::string>(fields_.at(FIELD::INTERFACE));  }
        std::string const& member() const       { return std::get<std::string>(fields_.at(FIELD::MEMBER));      }

    private:
        void serialize();

        DBusError extractArray(Signature const& s, int32_t index, DBusVariant& array);

        void insertValue(DBUS_TYPE type, void const* data, std::vector<uint8_t>& buffer);
        DBusError extractArgument(DBUS_TYPE type, void* data);
        DBusError checkSignature(DBUS_TYPE type);

        static uint32_t serialCounter_;

        struct Header header_;
        HeaderFields fields_;
        Signature signature_;       // DBus call signature.

        std::vector<uint8_t> headerBuffer_;  // DBus message header buffer.
        std::vector<uint8_t> body_;          // DBus message body buffer.

        uint32_t sign_pos_{0};
        uint32_t body_pos_{0};
    };
}

#include "DBusMessage.tpp"

#endif // DBUS_MESSAGE_H
