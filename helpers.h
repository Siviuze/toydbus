#ifndef DBUS_HELPERS_H
#define DBUS_HELPERS_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace dbus
{
    template<typename T>
    std::string hexDump(std::vector<T> const& buffer)
    {
        std::stringstream ss;
        uint8_t const* raw_buffer = buffer.data();
        uint32_t const size_in_bytes = buffer.size() * sizeof(T);
        
        if (buffer.empty())
        {
            ss << "empty" << std::endl;
            return ss.str();
        }
        
        for (uint32_t i = 0; i < size_in_bytes; ++i)
        {
            if ((i % 16) == 0)
            {
                if (i != 0)
                {
                    ss << std::endl;
                }
                ss << std::setfill('0') << std::setw(2) << std::hex << i;
                ss << ':';
            }
            if ((i % 8) == 0)
            {
                ss << " ";
            }
            ss << std::setfill('0') << std::setw(2) << std::hex << (uint32_t)raw_buffer[i];
            ss << " ";
        }
        ss << std::endl;
        
        return ss.str();
    }
    
    
    void updatePadding(int32_t padding_size, std::vector<uint8_t>& buffer);
    void align (uint32_t& position, uint32_t alignement);
}

#endif
