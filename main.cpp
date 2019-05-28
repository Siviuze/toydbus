#include <iostream>

#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/un.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>

#include <cstdint>
#include <variant>
#include <utility>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>


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


int main(int argc, char **argv) 
{
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket()");
        return -1;
    }
    
    constexpr struct sockaddr_un const SYSTEM_BUS{AF_UNIX, "/var/run/dbus/system_bus_socket"};
    //constexpr struct sockaddr_un const USER_BUS{AF_UNIX, "/run/user/1000/bus"};
    
    int c = connect(sockfd, (struct sockaddr*)&SYSTEM_BUS, sizeof(struct sockaddr_un));
    if (c < 0)
    {
        perror("connect()");
        return -1;
    }
    
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    // authentication
    {
        write(sockfd, "\0", 1);
        
        std::string ASK_AUTH("AUTH\r\n");
        int w = write(sockfd, ASK_AUTH.c_str(), ASK_AUTH.size());
        if (w < 0)
        {
            perror("write AUTH");
            return -1;
        }
        
        char buffer[4096];
        for (int32_t i=0; i<100; ++i)
        {
            int r = read(sockfd, buffer, 4096);
            if (r < 0)
            {
                usleep(1000 * 10);
                continue;
            }
            break;
        }
        
        printf("AUTH answer: %s\n", buffer);
        
        // create UID string to authenticate
        std::stringstream ss;
        ss << "AUTH EXTERNAL ";
        for (auto i : std::to_string(getuid()))
        {
            ss << std::hex << int(i);
        }
        ss << "\r\n";
        std::cout << ss.str() << std::endl;
        
        w = write(sockfd, ss.str().c_str(), ss.str().size());
        for (int32_t i=0; i<100; ++i)
        {
            int r = read(sockfd, buffer, 4096);
            if (r < 0)
            {
                usleep(1000 * 10);
                continue;
            }
            break;
        }
        printf("AUTH EXTERNAL answer: %s\n", buffer);
        
        std::string NEGO_FD("NEGOTIATE_UNIX_FD\r\n");
        w = write(sockfd, NEGO_FD.c_str(), NEGO_FD.size());
        memset(buffer, 0, 4096);
        for (int32_t i=0; i<100; ++i)
        {
            int r = read(sockfd, buffer, 4096);
            if (r < 0)
            {
                usleep(1000 * 10);
                continue;
            }
            break;
        }
        printf("NEGOCIATE_UNIX_FD answer: %s\n", buffer);
        
        std::string BEGIN("BEGIN\r\n");
        w = write(sockfd, BEGIN.c_str(), BEGIN.size());
        
        //return 0;
    }
    
    // call Hello
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
    
        
    return 0;
}
