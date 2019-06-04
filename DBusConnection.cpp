// C++
#include <regex>
#include <iostream>

// POSIX
#include <sys/socket.h> 
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>

#include "DBusConnection.h"
#include <iomanip>


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
        
        //-------- Send Hello() and get our unique name --------//
        DBusMessage hello;
        hello.prepareCall("org.freedesktop.DBus",
                          "/org/freedesktop/DBus",
                          "org.freedesktop.DBus",
                          "Hello");
        
        DBusMessage uniqueName;
        err = send(std::move(hello));
        if (err)
        {
            return err;
        }
        
        err = recv(uniqueName, 100ms);
        return err;
    }
    
    
    DBusError DBusConnection::recv(DBusMessage& msg, milliseconds timeout)
    {
        // Start with fixed length header part.
        DBusError err = readData(&msg.header_, sizeof(struct Header), timeout);
        if (err)
        {
            return err;
        }
        
        // Next read header fields.
        std::vector<uint8_t> fields;
        fields.resize(msg.header_.fields_size);
        err = readData(fields.data(), fields.size(), timeout);
        if (err)
        {
            return err;
        }

        // Read message body. 
        msg.body_.resize(msg.header_.size);
        err = readData(msg.body_.data(), msg.body_.size(), timeout);
        if (err)
        {
            return err;
        }
        
        //TODO parse msgFields to populate reply fields member.
        return ESUCCESS;
    }
    
    
    DBusError DBusConnection::send(DBusMessage&& msg)
    {
        if (not name_.empty())
        {
            // Add sender filed with our name if we know it (if not, probable the Hello() message).
            msg.fields_.value.push_back({FIELD::SENDER, {name_}});
        }
        
        msg.serialize();
        /*
        for (int i=0; i<msg.headerBuffer_.size(); ++i)
        {
            if ((i%16) == 0)
            {
                printf("\n");
            }
            printf("0x%02x ", msg.headerBuffer_[i]);
        }
        TODO add a dump method to help debugging.
        */
        
        
        // Send message.
        return writeData(msg.headerBuffer_.data(), msg.headerBuffer_.size(), 100ms);
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
    
    
    DBusError DBusConnection::readData(void* data, uint32_t data_size, milliseconds timeout)
    {
        auto start_timestamp = steady_clock::now();
        
        uint32_t to_read = data_size;
        uint32_t position = 0;
        uint8_t* buffer = reinterpret_cast<uint8_t*>(data);
        while (to_read > 0)
        {
            auto now = steady_clock::now();
            milliseconds spent = duration_cast<milliseconds>(now - start_timestamp);
            if ((timeout - spent) < 0ms)
            {
                return EERROR("timeout");
            }
            
            int r = read(fd_, buffer + position, to_read);
            if (r < 0)
            {
                if (errno == EAGAIN)
                {
                    usleep(1000);
                    continue;
                }
                return EERROR(strerror(errno));
            }
            
            to_read -= r;
            position += r;
        }
        
        return ESUCCESS;
    }
    
    
    DBusError DBusConnection::writeData(void const* data, uint32_t data_size, milliseconds timeout)
    {
        auto start_timestamp = steady_clock::now();
        
        uint32_t to_write = data_size;
        uint32_t position = 0;
        uint8_t const* buffer = reinterpret_cast<uint8_t const*>(data);
        while (to_write > 0)
        {
            auto now = steady_clock::now();
            milliseconds spent = duration_cast<milliseconds>(now - start_timestamp);
            if ((timeout - spent) < 0ms)
            {
                return EERROR("timeout");
            }
            
            int r = write(fd_, buffer + position, to_write);
            if (r < 0)
            {
                if (errno == EAGAIN)
                {
                    usleep(1000);
                    continue;
                }
                return EERROR(strerror(errno));
            }
            
            to_write -= r;
            position += r;
        }
        
        return ESUCCESS;
    }
}
