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
