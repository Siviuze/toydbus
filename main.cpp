#include <iostream>

#include "DBusConnection.h"
#include "DBusMessage.h"

using namespace dbus;


int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    DBusConnection bus;
    DBusError err = bus.connect(DBusConnection::BUS_SYSTEM);
    if (err)
    {
        err.what();
        return 1;
    }

    DBusMessage msg;
    msg.prepareCall("org.freedesktop.UDisks2", "/org/freedesktop/UDisks2", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");

    err = bus.send(std::move(msg));
    if (err)
    {
        err.what();
        return 1;
    }

    for (int i=0 ; i<100; ++i)
    {
        DBusMessage answer;
        err = bus.recv(answer, 100ms);
        if (err)
        {
            err.what();
            return 1;
        }
        std::cout <<  answer.dump();
    }
	
    return 0;
}
