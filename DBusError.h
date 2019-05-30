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

#ifndef DBUS_ERROR_H
#define DBUS_ERROR_H

#include <string>
#include <cstdint>

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
