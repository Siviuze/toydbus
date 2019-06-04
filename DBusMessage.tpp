namespace dbus
{
    template<typename T>
    void DBusMessage::addArgument(T const& arg)
    {
        signature_ += dbusType<T>();
        insertValue(dbusType<T>(), &arg, body_);
    }
    
    
    template<typename K, typename V>
    void DBusMessage::addArgument(Dict<K, V> const& arg)
    {
        signature_ += DBUS_TYPE::ARRAY;
        signature_ += DBUS_TYPE::DICT_BEGIN;
        signature_ += dbusType<K>();
        signature_ += dbusType<V>();
        signature_ += DBUS_TYPE::DICT_END;
        
        for (auto& i : arg.value)
        {
            updatePadding(8, body_); // dict entry aligned on 8 bytes.
            
            // insert key
            insertValue(dbusType<K>(), &i.first, body_);
            
            // insert value
            insertValue(dbusType<V>(), &i.second, body_);
        }
    }
    
    
    template<typename T>
    DBusError DBusMessage::extractArgument(T& arg)
    {
        DBusError err = checkSignature(dbusType<T>());
        if (err)
        {
            return err;
        }

        return extractArgument(dbusType<T>(), &arg);
    }
    
    
    template<typename K, typename V> 
    DBusError DBusMessage::extractArgument(Dict<K, V> const& arg)
    {
    return EERROR("Not implemented");
    }
}
