#ifndef DBUS_CONNECTION_H
#define DBUS_CONNECTION_H

// C++
#include <chrono>

#include "DBusMessage.h"

namespace dbus
{
    using namespace std::chrono;
    
    class DBusConnection
    {
    public:
        enum BUS_TYPE
        {
            BUS_SYSTEM,
            BUS_SESSION,
            BUS_USER
        };
        
        DBusError connect(BUS_TYPE bus);
        DBusError send(DBusMessage&& msg);
        DBusError recv(DBusMessage& msg, milliseconds timeout);
        
    private:
        DBusError initSocket(BUS_TYPE bus);
        DBusError readAuth(std::string& reply, milliseconds timeout);
        DBusError writeAuthRequest(std::string const& request);
        
        DBusError readData(void* data, uint32_t data_size, milliseconds timeout);
        DBusError writeData(void const* data, uint32_t data_size, milliseconds timeout);
        
        int fd_;
        std::string name_; // our unique name on the bus.
    };
}

#endif // DBUSCONNECTION_H
