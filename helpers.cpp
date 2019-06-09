#include "helpers.h"


namespace dbus
{
    void updatePadding(int32_t padding_size, std::vector<uint8_t>& buffer)
    {
        if (buffer.size() % padding_size)                                  // is padding needed ?
        {
            int32_t padding = padding_size - buffer.size() % padding_size; // compute padding
            buffer.resize(buffer.size() + padding);                        // add padding
        }
    }
    
    
    void align(uint32_t& position, uint32_t alignment)
    {
        if (position % alignment)
        {
            position += alignment - (position % alignment);
        }
    }
}
