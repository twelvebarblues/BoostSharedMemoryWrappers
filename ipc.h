#ifndef IPC_H
#define IPC_H

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <iostream>

namespace bip = boost::interprocess;

namespace ipc
{
    using segment         = bip::managed_shared_memory;
    using segment_manager = bip::managed_shared_memory::segment_manager;

    template<typename T>
    using alloc = bip::allocator<T, segment_manager>;

    template<typename T>
    using vector = bip::vector<T, alloc<T>>;

    template<typename K, typename V>
    using pAlloc = alloc<std::pair<const K, V>>;

    template<typename K, typename V, typename Compare = std::less<K>>
    using map = bip::map<K, V, Compare, pAlloc<K, V>>;

    using string = bip::basic_string<char, std::char_traits<char>, alloc<char>>;

    /**
     * SharedBase is a base class for all shared memory wrappers.
     */
    template<typename T>
    class SharedBase
    {
      public:
        /**
         * Base class for shared memeory object
         *
         * @param name the name of the shared memory object
         * @param owner flag indicating if the object should be created or found
         */
        SharedBase(const char* name, bool owner) : _owner(owner), _name(name)
        {
            if (_owner)
            {
                bip::shared_memory_object::remove(_name);
            }
        }

        /**
         * Destroy this shared memory object if this instance
         * was the one that created it.
         */
        ~SharedBase()
        {
            if (_owner)
            {
                std::cerr << __FUNCTION__ << " " << _name << std::endl;
                bip::shared_memory_object::remove(_name);
            }
        }

        /**
         * Return the pointer to the internal object
         *
         * @returns the pointer to the internal object
         */
        T* operator->()
        {
            return _object;
        }

        /**
         * Return a reference to the internal object
         *
         * @returns a reference to the internal object
         */
        T& reference()
        {
            return *_object;
        }

      protected:
        bool _owner;
        const char* _name;
        T* _object = nullptr;
    };

    /**
     * SharedMemory creates a named shared memory segment.  This class is mainly
     * useful as a subclass, where the subclass would contain other shared
     * memory objects.
     *
     * It contains two constructors, one used to create the shared memory
     * segment and the other find it.
     */
    class SharedMemory : public SharedBase<bip::managed_shared_memory>
    {
      public:
        /**
         * Create open a shared memory segment
         *
         * @param name the name of the segment
         * @param size the requested size of the segment
         */
        SharedMemory(const char* name, unsigned int size)
            : SharedBase<bip::managed_shared_memory>(name, true),
              _segment(bip::create_only, name, size)
        {
            this->_object = &_segment;
        }

        /**
         * Open the given named shared memory segment
         *
         * @param name the name of the segment to open
         */
        SharedMemory(const char* name)
            : SharedBase<bip::managed_shared_memory>(name, false),
              _segment(bip::open_only, name)
        {
        }

        /**
         * Helper method to construct an allocator of the
         * specified type from this segment manager.
         *
         * @returns the an allocator
         */
        template<typename T>
        ipc::alloc<T> getAllocator()
        {
            return ipc::alloc<T>(_segment.get_segment_manager());
        }

      protected:
        bip::managed_shared_memory _segment;
    };

    /**
     * SharedMutex is a wrapper for a boost::interprocess mutexes
     */
    template<typename T>
    class SharedMutex : public SharedBase<T>
    {
      public:
        using sMutex = T;

        /**
         * Constructor for creating or finding a shared memory object.
         *
         * @param name the name of the shared memory object
         * @param segment a reference to the shared memory segment
         * @param owner flag indicating if the object should be created or found
         */
        SharedMutex(const char* name, segment& segment, bool owner)
            : SharedBase<sMutex>(name, owner)
        {
            if (owner)
            {
                this->_object = segment.construct<sMutex>(name)();
            }
            else
            {
                this->_object = segment.find<sMutex>(name).first;
            }
        }
    };

    /**
     * SharedVector is a wrapper for a boost::iterprocess::vector.
     */
    template<typename Element>
    class SharedVector : public SharedBase<ipc::vector<Element>>
    {
      public:
        using sVector = ipc::vector<Element>;

        /**
         * Constructor for creating a shared memory object.
         *
         * @param reserve the number of elements to reserve
         * @param name the name of the shared memory object to create
         * @param segment a reference to the shared memory segment
         */
        SharedVector(unsigned int reserve, const char* name, segment& segment)
            : SharedBase<sVector>(name, true)
        {
            ipc::alloc<sVector> eAlloc(segment.get_segment_manager());
            this->_object = segment.construct<sVector>(name)(eAlloc);
            this->_object->reserve(reserve);
        }

        /**
         * Constructor for finding an existing shared memory object.
         *
         * @param name the name of the shared memory object to find
         * @param segment a reference to the shared memory segment
         */
        SharedVector(const char* name, segment& segment)
            : SharedBase<sVector>(name, false)
        {
            this->_object = segment.find<sVector>(name).first;
        }
    };

    /**
     * SharedMap is a wrapper for a boost::imterprocess::map.
     */
    template<typename K, typename V, typename Compare = std::less<K>>
    class SharedMap : public SharedBase<ipc::map<K, V, Compare>>
    {
      public:
        using sMap = ipc::map<K, V, Compare>;

        /**
         * Constructor for creating or finding a shared memory object.
         *
         * @param name the name of the shared memory object to create
         * @param segment a reference to the shared memory segment
         * @param owner flag indicating if the object should be created or found
         */
        SharedMap(const char* name, segment& segment, bool owner)
            : SharedBase<sMap>(name, owner)
        {
            if (owner)
            {
                auto a        = pAlloc<K, V>(segment.get_segment_manager());
                this->_object = segment.construct<sMap>(name)(Compare(), a);
            }
            else
            {
                this->_object = segment.find<sMap>(name).first;
            }
        }
    };
}

#endif
