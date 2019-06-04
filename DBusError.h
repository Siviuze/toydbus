#ifndef DBUS_ERROR_H
#define DBUS_ERROR_H

#include <cstdint>
#include <string>

namespace dbus
{
    #define ESUCCESS DBusError()
    #define EERROR(err) DBusError(err, __FUNCTION__, __FILE__, __LINE__)
    
    class DBusError
    {
    public:
        DBusError()  = default;
        ~DBusError() = default;
        
        DBusError(DBusError&& other) = default;
        DBusError& operator=(DBusError&& other) = default;
        
        DBusError(DBusError const& other) = delete;
        DBusError& operator=(DBusError const& other) = delete;
                
        DBusError(std::string const& message, 
                  std::string const& function, 
                  std::string const& file, 
                  int32_t line);

        operator bool() const { return isError_; }
        std::string what() const;
        
    private:
        bool isError_{false};
        std::string error_;
        std::string function_;
        std::string file_;
        int32_t line_;
    };
}

#endif // DBUSERROR_H
