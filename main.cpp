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
        return err;
    }

    return 0;
}
