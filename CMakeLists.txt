cmake_minimum_required(VERSION 3.0)

project(dbus)
set (CMAKE_CXX_STANDARD 17)

add_executable(dbus main.cpp)

install(TARGETS dbus RUNTIME DESTINATION bin)
