#include "DBusError.h"

namespace dbus
{
    DBusError::DBusError(std::string const& message, 
                         std::string const& function, 
                         std::string const& file, 
                         int32_t line)
        : isError_(true)
        , error_(message)
        , function_(function)
        , file_(file)
        , line_(line)
    { }
    
    std::string DBusError::what() const
    {
        if (not isError_)
        {
            return "success";
        }
        return function_ + ": " + error_ + " (" + file_ + ":" + std::to_string(line_) + ")";
    }  
}
