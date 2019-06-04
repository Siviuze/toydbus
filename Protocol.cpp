#include "Protocol.h"

namespace dbus
{
    std::string str(DBUS_TYPE type)
    {
        std::string str(1, static_cast<char>(type));
        return str;
    }
    
    
    std::string str(FIELD type)
    {
        switch (type)
        {
            case FIELD::PATH:         { return "Path";         }
            case FIELD::INTERFACE:    { return "Interface";    }
            case FIELD::MEMBER:       { return "Member";       }
            case FIELD::ERROR_NAME:   { return "Error name";   }
            case FIELD::REPLY_SERIAL: { return "Reply serial"; }
            case FIELD::DESTINATION:  { return "Destination";  }
            case FIELD::SENDER:       { return "Sender";       }
            case FIELD::SIGNATURE:    { return "Signature";    }
            case FIELD::UNIX_FDS:     { return "UNIX FDS";     }
            default:
            case FIELD::INVALID:      { return "Invalid";      }
        }
    }
    
    
    std::string str(MESSAGE_TYPE type)
    {
        switch (type)
        {
            case MESSAGE_TYPE::INVALID:       { return "Invalid";      }
            case MESSAGE_TYPE::METHOD_CALL:   { return "Method call";  }
            case MESSAGE_TYPE::METHOD_RETURN: { return "Method reply"; }
            case MESSAGE_TYPE::ERROR:         { return "Error";        } 
            case MESSAGE_TYPE::SIGNAL:        { return "Signal";       }
            default:                          { return "Unknown";      }
        }
    }
    
    
    std::string str(ENDIANNESS endianness)
    {
        switch (endianness)
        {
            case ENDIANNESS::BIG:    { return "Big endian";    }
            case ENDIANNESS::LITTLE: { return "Little endian"; }
            default:                 { return "unknown";       }
        }
    }
    
    
    Signature& Signature::operator+=(DBUS_TYPE type)
    {
        *this += str(type);
        return *this;
    }
    
    
    bool Signature::operator==(DBUS_TYPE type)
    {
        return (*this == str(type));
    }
    
    
    bool Signature::operator!=(DBUS_TYPE type)
    {
        return not (*this == type);
    }
}
