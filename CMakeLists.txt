cmake_minimum_required(VERSION 3.0)

project(dbus)
set (CMAKE_CXX_STANDARD 17)
add_definitions(-Wall -Wextra) # enable common warnings

set (SRCS   "${CMAKE_CURRENT_SOURCE_DIR}/DBusError.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/DBusConnection.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/DBusMessage.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_executable(dbus ${SRCS})

install(TARGETS dbus RUNTIME DESTINATION bin)
