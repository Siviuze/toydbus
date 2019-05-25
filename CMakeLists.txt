cmake_minimum_required(VERSION 3.0)

project(dbus)

add_executable(dbus main.cpp)

install(TARGETS dbus RUNTIME DESTINATION bin)
