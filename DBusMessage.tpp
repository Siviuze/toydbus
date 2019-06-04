namespace dbus
{
    // Basic types
    template<> void DBusMessage::addArgument<int16_t>(int16_t const& arg);
    template<> void DBusMessage::addArgument<uint16_t>(uint16_t const& arg);
    template<> void DBusMessage::addArgument<int32_t>(int32_t const& arg);
    template<> void DBusMessage::addArgument<uint32_t>(uint32_t const& arg);
    template<> void DBusMessage::addArgument<int64_t>(int64_t const& arg);
    template<> void DBusMessage::addArgument<uint64_t>(uint64_t const& arg);
    template<> void DBusMessage::addArgument<bool>(bool const& arg);
    template<> void DBusMessage::addArgument<uint8_t>(uint8_t const& arg);

    // String like types
    template<> void DBusMessage::addArgument<std::string>(std::string const& arg);
    template<> void DBusMessage::addArgument<ObjectPath>(ObjectPath const& arg);
    template<> void DBusMessage::addArgument<Signature>(Signature const& arg);
    
    // containers types
    template<> void DBusMessage::addArgument<Variant>(Variant const& arg);
    
    
    template<> DBusError DBusMessage::extractArgument(std::string& arg);
}
