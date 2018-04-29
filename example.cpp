#include "ipc.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <string>

/**
 * This program is a simple example of the shared memory
 * wrappers defined in "ipc.h".  
 */

// A simple shared memory object
struct employee
{
    // boost::interprocess::string required
    ipc::string _name;
    int _empNum;
};

// 
struct SC : public ipc::SharedMemory
{
    // Construct this object, creating all the shared
    // memory objects.  This is the "server".
    SC(unsigned int size)
        : ipc::SharedMemory("EMP_SEGMENT", size),
          _vector(10, "EMP_VECTOR", _segment),
          _map("EMP_MAP", _segment, true),
          _mtx("EMP_MTX", _segment, true)
    {
    }

    // Construct this object, initialize references to 
    // existing shared memory objects. This is the "client".
    SC()
        : ipc::SharedMemory("EMP_SEGMENT"),
          _vector("EMP_VECTOR", _segment),
          _map("EMP_MAP", _segment, false),
          _mtx("EMP_MTX", _segment, false)
    {
    }

    void show()
    {
        // Lock while reading, note we used a recursive mutex
        bip::scoped_lock<bip::interprocess_recursive_mutex> lock(_mtx.reference());

        // Use the reference() method in range based for loops
        std::cerr << "\nvector:  ";
        for (auto& e : _vector.reference())
        {
            std::cerr << "(" << e._name.c_str() << " " << e._empNum << ") ";
        }
        std::cerr << "\n";

        // Showing that you can use the objects as a pointer
        auto p = _map->begin();
        auto end = _map->end();
        std::cerr << "map:     ";
        while (p != end)
        {
            std::cerr << "[" << p->first << "]=" << p->second << " ";
            ++p;
        }
        std::cerr << "\n\n";
    }

    ipc::SharedMap<ipc::string, int> _map;
    ipc::SharedVector<employee> _vector;
    ipc::SharedMutex<bip::interprocess_recursive_mutex> _mtx;
};

int main(int argc, char* argv[])
{
    // Run the program with no arguments for the "server", and with 
    // one (or more) arguments to be a "client".  Note: if you kill 
    // the server before the client, bad things happen.
    if (argc == 1)
    {
        std::cout << "In parent" << std::endl;

        SC sm(65536);

        int num = 1000;
        auto aa = sm.getAllocator<char>();

        // Add an initial element to the vector and map
        sm._vector->push_back({ipc::string("Assad", aa), num});
        sm._map->insert(std::pair<ipc::string, int>(ipc::string("Assad", aa), num++));

        for (auto& n : {"Putin", "Kim", "Hillary", "Jong", "Obama", "Un", "Comey" })
        {
            // Give your peers a chance to acquire the lock
            std::this_thread::sleep_for(std::chrono::microseconds(1));

            // Acquire the lock
            bip::scoped_lock<bip::interprocess_recursive_mutex> lock(sm._mtx.reference());

            // Show what we've got so far
            sm.show();

            // Ask for user input while holding to lock to block the client from reading
            std::cout << "Enter zero to terminate: ";
            int input = 0;
            std::cin >> input;
            if (input == 0) break;

            // The operator->() is overloaded in SharedVector and SharedMap so 
            // the full boost objects API is available using pointer semantics
            sm._vector->push_back({ipc::string(n, aa), num});
            sm._map->insert(std::pair<ipc::string, int>(ipc::string(n, aa), num++));
        }

        for (auto& e : sm._vector.reference())
        {
            std::cerr << e._name.c_str() << " ";
        }
        std::cerr << "\n";
    }
    else
    {
        std::cout << "In child" << std::endl;
        SC sm;
        while (true)
        {
            // Give your peers a chance to acquire the lock
            std::this_thread::sleep_for(std::chrono::microseconds(1));

            // Acquire the lock
            bip::scoped_lock<bip::interprocess_recursive_mutex> lock(
                sm._mtx.reference());
            sm.show();

            // Grab the top vector element and add a "client" version
            auto e = sm._vector->back();
            std::string n = e._name.c_str();
            n += "-client";

            auto aa = sm.getAllocator<char>();
            sm._vector->push_back({ipc::string(n.c_str(), aa), e._empNum});
            sm._map->insert(
                std::pair<ipc::string, int>(ipc::string(n.c_str(), aa), e._empNum));
        }
    }
    return 0;
};
