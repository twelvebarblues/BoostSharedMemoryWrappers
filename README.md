# BoostSharedMemoryWrappers
This project creates wrappers for some of the boost::interprocess objects for shared memory. 

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
