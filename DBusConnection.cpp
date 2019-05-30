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

// C++
#include <regex>
#include <iostream>

// POSIX
#include <sys/socket.h> 
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>

#include "DBusConnection.h"


namespace dbus
{
    namespace auth
    {
        std::string const AUTH      {"AUTH"};
        std::string const EXTERNAL  {"EXTERNAL"};
        std::string const REJECTED  {"REJECTED"};
        std::string const ENDLINE   {"\r\n"};
        std::string const NEGOCIATE {"NEGOTIATE_UNIX_FD"};
        std::string const BEGIN     {"BEGIN"};
    }
    
    DBusError DBusConnection::connect(BUS_TYPE bus)
    {
        //-------- connect socket --------//
        DBusError err = initSocket(bus);
        if (err)
        {
            return err;
        }
        
        //-------- start authentication --------//
        // discover supported mode
        err = writeAuthRequest(auth::AUTH);
        if (err)
        {
            return err;
        }
        
        std::string reply;
        err = readAuth(reply, 2000ms);
        if (err)
        {
            return err;
        }
        
        std::regex re{"\\s+"};
        std::vector<std::string> supported_auth
        {
            std::sregex_token_iterator(reply.begin(), reply.end(), re, -1),
            std::sregex_token_iterator()
        };
        
        std::cout << "Supported AUTH mode: " << std::endl;
        for (auto i : supported_auth)
        {
            if (i == auth::REJECTED)
            {
                continue;
            }
            std::cout << i << std::endl;
        }
        
        //TODO select mode
        
        //-------- EXTERNAL mode --------//
        
        // create UID string to authenticate
        std::stringstream ss;
        ss << "AUTH EXTERNAL ";
        for (auto i : std::to_string(getuid()))
        {
            ss << std::hex << int(i);
        }
        std::cout << ss.str() << std::endl;

        err = writeAuthRequest(ss.str());
        if (err)
        {
            return err;
        }
        
        err = readAuth(reply, 2000ms);
        if (err)
        {
            return err;
        }
        std::cout << reply << std::endl;
        
        //-------- Nego UNIX FD --------//
        err = writeAuthRequest(auth::NEGOCIATE);
        if (err)
        {
            return err;
        }

        err = readAuth(reply, 2000ms);
        if (err)
        {
            return err;
        }
        std::cout << reply << std::endl;
        
        //-------- We are ready: BEGIN --------//
        err = writeAuthRequest(auth::BEGIN);
        if (err)
        {
            return err;
        }
        
        return ESUCCESS;
    }
    
    
    DBusError DBusConnection::initSocket(BUS_TYPE bus)
    {
        fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd_ < 0)
        {
            return EERROR(strerror(errno));
        }
        
        int rc = -1;
        switch (bus)
        {
            case BUS_SYSTEM:
            {
                constexpr struct sockaddr_un const SYSTEM_BUS {AF_UNIX, "/var/run/dbus/system_bus_socket" };
                rc = ::connect(fd_, (struct sockaddr*)&SYSTEM_BUS, sizeof(struct sockaddr_un));
                break;
            }
            
            case BUS_SESSION:
            {
                return EERROR("Not implemented");
            }
            
            case BUS_USER:
            {
                return EERROR("Not implemented");
            }
        }
        if (rc < 0)
        {
            return EERROR(strerror(errno));
        }
        
        // set socket non blocking
        int flags = fcntl(fd_, F_GETFL, 0);
        if (flags < 0)
        {
            return EERROR(strerror(errno));
        }
        
        rc = fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
        if (rc < 0)
        {
            return EERROR(strerror(errno));
        }
        
        rc = write(fd_, "\0", 1);
        if (rc < 0)
        {
            return EERROR(strerror(errno));
        }
        
        return ESUCCESS;
    }
    
    
    DBusError DBusConnection::writeAuthRequest(std::string const& request)
    {
        std::string req = request + auth::ENDLINE;
        int32_t rc = write(fd_, req.c_str(), req.size());
        if (rc < 0)
        {
            return EERROR(strerror(errno));
        }
        
        return ESUCCESS;
    }
    
    
    DBusError DBusConnection::readAuth(std::string& reply, milliseconds timeout)
    {
        reply.clear();
        auto start = steady_clock::now();
        
        while (true)
        {
            auto now = steady_clock::now();
            milliseconds spent = duration_cast<milliseconds>(now - start);
            if ((timeout - spent) < 0ms)
            {
                return EERROR("timeout");
            }
            
            uint8_t buffer[4096];
            int r = read(fd_, buffer, 4096);
            if (r < 0)
            {
                if (errno == EAGAIN)
                {
                    usleep(1000);
                    continue;
                }
                return EERROR(strerror(errno));
            }
            
            reply.insert(reply.end(), buffer, buffer + r);
            if (std::equal(auth::ENDLINE.rbegin(), auth::ENDLINE.rend(), reply.rbegin()))
            {
                return ESUCCESS;
            }
        }
    }
}
