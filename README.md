# BoostSharedMemoryWrappers
This project creates wrappers for some of the boost::interprocess objects for shared memory. The wrappers are intended to be used in a client-server fashion, where the server creates the initial shared memory segment and any objects, then the client attaches.  This is supported by having distinct constructors, one each for a client and server, or by passing an __owner__ flag to the single constructor.  A simple application can just inherit from the __ipc::SharedMemory__ class and use the same two constructor pattern. 
```
#include "ipc.h"
#include <iostream>
#include <string>

struct employee
{
    ipc::string _name;
    int _empNum;
};

struct SC : public ipc::SharedMemory
{
    // Server constructor
    SC(unsigned int size) : ipc::SharedMemory("SEGMENT_NAME", size), 
        _vector(10, "VECTOR_NAME", _segment)
    {
    }

    // Client constuctor
    SC() : ipc::SharedMemory("SEGMENT_NAME"), _vector("VECTOR_NAME", _segment)
    {
    }

    ipc::SharedVector<employee> _vector;
};

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        std::cout << "In parent" << std::endl;
        SC sm(65536);

        auto aa = sm.getAllocator<char>();
        sm._vector->push_back({ipc::string("The Dude", aa), 42});
        char c[32];
        std::cin.getline(c, 32);
    }
    else
    {
        std::cout << "In child" << std::endl;
        SC sm;
        std::cout << "Found: " << sm._vector->back()._name.c_str() << "\n";
    }
    return 0;
};

```


## System Requirements
This code was developed on Ubuntu 16.04 using the following:
* boost 1.58
* G++ 5.4.0

## Building
The code is header only, to build the example:
```
g++ example.cpp -o example -std=c++11 -lrt -lpthread
```
## ToDo
* Add support for condition variables
* Add support for boost multi-index
