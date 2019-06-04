#### Purpose ####
ToyDBus is an implementation of DBus protocol (client side) in C++17.

#### Why a new DBus stack ####
I wanted to play with new C++ features, and since the official DBus stack is hard to use (as written in its API documentation: `If you use this low-level API directly, you're signing up for some pain`), I decide to start a toy project with a DBus client API as a target.

#### How to build it ####
The project build system is CMake:
```
mkdir build
cd build
cmake ..
make
```

