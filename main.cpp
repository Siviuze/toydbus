#include <iostream>

#include <sys/types.h> 


#include <netdb.h>

#include <vector>
#include <fstream>
#include <sstream>


#include "DBusConnection.h"

using namespace dbus;


int main(int argc, char **argv) 
{
    DBusConnection bus;
    DBusError err = bus.connect(DBusConnection::BUS_SYSTEM);
    if (err)
    {
        std::cout << err.what() << std::endl;
        return err;
    }
    
    
    int sockfd;
    // authentication
    {

        
        //return 0;
    }
    
    // call Hello
    /*
    Header header{ENDIANNESS::LITTLE, MESSAGE_TYPE::METHOD_CALL, 
        0, 1, 0, 1, 0};
      
    HeaderFields fields
    {
        {FIELD::DESTINATION, "org.freedesktop.DBus"}, 
        {FIELD::PATH, ObjectPath("/org/freedesktop/DBus")}, 
        {FIELD::INTERFACE, "org.freedesktop.DBus"}, 
        {FIELD::MEMBER, "Hello"},
    };
    
    std::vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(header.endianness));
    msg.push_back(static_cast<uint8_t>(header.type));
    msg.push_back(static_cast<uint8_t>(header.flags));
    msg.push_back(static_cast<uint8_t>(header.version));
    
    // body size
    msg.push_back((header.size & 0x000000FF) >> 0);
    msg.push_back((header.size & 0x0000FF00) >> 8);
    msg.push_back((header.size & 0x00FF0000) >> 16);
    msg.push_back((header.size & 0xFF000000) >> 24);
    
    // serial
    msg.push_back((header.serial & 0x000000FF) >> 0);
    msg.push_back((header.serial & 0x0000FF00) >> 8);
    msg.push_back((header.serial & 0x00FF0000) >> 16);
    msg.push_back((header.serial & 0xFF000000) >> 24);
    
    // fields size 
    msg.push_back(0);
    msg.push_back(0);
    msg.push_back(0);
    msg.push_back(0);
    int position = msg.size();
    
    for (auto it : fields)
    {
        if (msg.size() % 8)
        {
            int padding = 8 - msg.size() % 8;     // variant/struct aligned on 8 bytes
            msg.resize(msg.size() + padding); // add padding
        }
        
        msg.push_back(static_cast<uint8_t>(it.first)); // field type
        msg.push_back(1);                              // signature size of a variant
        switch (it.second.index())
        {
            case 0:
            {
                // std::string
                msg.push_back(static_cast<uint8_t>(DBUS_TYPE::STRING));
                msg.push_back('\0');
                
                int size = std::get<std::string>(it.second).size();
                msg.push_back((size & 0x000000FF) >> 0);
                msg.push_back((size & 0x0000FF00) >> 8);
                msg.push_back((size & 0x00FF0000) >> 16);
                msg.push_back((size & 0xFF000000) >> 24);
                msg.insert(msg.end(), std::get<std::string>(it.second).begin(), std::get<std::string>(it.second).end());
                msg.push_back('\0');
                
                break;
            }
            case 1:
            {
                // uint32_t
                msg.push_back(static_cast<uint8_t>(DBUS_TYPE::UINT32));
                msg.push_back('\0');
                break;
            }
            case 2:
            {
                // object path
                msg.push_back(static_cast<uint8_t>(DBUS_TYPE::PATH));
                msg.push_back('\0');
                
                int size = std::get<ObjectPath>(it.second).value.size();
                msg.push_back((size & 0x000000FF) >> 0);
                msg.push_back((size & 0x0000FF00) >> 8);
                msg.push_back((size & 0x00FF0000) >> 16);
                msg.push_back((size & 0xFF000000) >> 24);
                msg.insert(msg.end(), std::get<ObjectPath>(it.second).value.begin(), std::get<ObjectPath>(it.second).value.end());
                msg.push_back('\0');
                
                break;
            }
            case 3:
            {
                // signature
                msg.push_back(static_cast<uint8_t>(DBUS_TYPE::SIGNATURE));
                msg.push_back('\0');
                break;
            }
        }
    }
    
    int array_size = msg.size() - position;
    msg[12] = ((array_size & 0x000000FF) >> 0);
    msg[13] = ((array_size & 0x0000FF00) >> 8);
    msg[14] = ((array_size & 0x00FF0000) >> 16);
    msg[15] = ((array_size & 0xFF000000) >> 24);
    
    if (msg.size() % 8)
    {
        int padding = 8 - msg.size() % 8; // variant/struct aligned on 8 bytes
        msg.resize(msg.size() + padding); // add padding
    }
    
    

    std::ofstream outfile ("hexa.txt",std::ofstream::binary);
    for (auto const& byte : msg)
    {
        outfile << byte;
    }
    
    for (int i=0; i<msg.size(); ++i)
    {
        if ((i%16) == 0)
        {
            printf("\n");
        }
        printf("0x%02x ", msg[i]);
    }
    
    
    printf("\n----------------\n");
    printf("Send Hello!\n");
    int w = write(sockfd, msg.data(), msg.size());
    if (w < 0)
    {
        perror("write()");
        return -1;
    }
    
    Header replyHeader{ENDIANNESS::BIG, MESSAGE_TYPE::INVALID, 0, 0, 0, 0, 0};
    int r = read(sockfd, &replyHeader, sizeof(replyHeader));
    for (int32_t i=0; i<100; ++i)
    {
        int r = read(sockfd, &replyHeader, sizeof(replyHeader));
        if (r < 0)
        {
            usleep(1000 * 10);
            continue;
        }
        break;
    }
    
    printf("receive an answer with an array of %d bytes\n", replyHeader.fields_size);
    */
        
    return 0;
}
