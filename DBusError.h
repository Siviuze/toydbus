#ifndef DBUS_ERROR_H
#define DBUS_ERROR_H

#include <cstdint>
#include <string>
#include <vector>

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
        
        DBusError& operator+=(DBusError&& other);

        operator bool() const { return isError_; }
        void what() const;
        
    private:
        bool isError_{false};
        std::string what_{"success"};
        std::vector<std::string> backtrace_;
    };
}

#endif // DBUSERROR_H
