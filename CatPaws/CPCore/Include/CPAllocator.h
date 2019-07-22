#ifndef CATPAWS_CPCORE_CPMALLOC_H_
#define CATPAWS_CPCORE_CPMALLOC_H_

//Before do real thing, I just followed Tiago Costa's GameDev Tutorial
//https://www.gamedev.net/articles/programming/general-and-gameplay-programming/c-custom-memory-allocation-r3010/

class CPAllocator
{
public:
    CPAllocator() = delete;

    CPAllocator(std::size_t size, void* start)
        : start_{ start }, 
        size_{ size }, 
        used_memory_ { 0 }, 
        num_allocations_ { 0 }
    {}

    virtual ~CPAllocator()
    {
        assert(num_allocations_ == 0 && used_memory_ == 0);
        start_ = nullptr;
        size_ = 0;
    }

    virtual void* Allocate(std::size_t size, uint8_t alignment = 4) = 0;
    virtual void Deallocate(void* p) = 0;
    void* Start() const { return start_; }
    std::size_t Size() const { return size_; }
    std::size_t UsedMemory() const { return used_memory_; }
    std::size_t NumAllocation() const { return num_allocations_; }

protected:
    void* start_;
    std::size_t size_;
    std::size_t used_memory_;
    std::size_t num_allocations_;
};

namespace Allocator
{
    template<class T> T* AllocateNew(CPAllocator& allocator)
    {
        return new (allocator.Allocate(sizeof(T), alignof(T))) T;
    }

    template<class T> T* AllocateNew(CPAllocator& allocator, const T& t)
    {
        return new (allocator.Allocate(sizeof(T), alignof(T))) T(t);
    }

    template<class T> void DeallocateDelete(CPAllocator& allocator, T& object)
    {
        object.~T();
        allocator.Deallocate(&object);
    }

    template<class T> T* AllocateArray(CPAllocator& allocator, std::size_t length)
    {
        assert(length != 0);
        uint8_t headersize = sizeof(std::size_t) / sizeof(T);

        if (sizeof(std::size_t) % sizeof(T) > 0)
        {
            ++headersize;
        }

        T* p = ((T*)allocator.Allocate(sizeof(T) * 
            (length + headersize), alignof(T))) + headersize;

        *( ((std::size_t*)p) - 1 ) = length;

        for (std::size_t i = 0; i < length; i++)
        {
            new (&p[i]) T;
        }

        return p;
    }

    template <class T> void DeallocateArray(CPAllocator& allocator, T* array)
    {
        assert(array != nullptr);
        std::size_t length = *(((std::size_t*)array)-1);

        for (std::size_t i = 0; i < length; i++)
        {
            array[i].~T();
        }

        uint8_t headersize = sizeof(std::size_t) / sizeof(T);

        if (sizeof(std::size_t) % sizeof(T) > 0)
        {
            headersize++;
        }

        allocator.Deallocate(array - headersize);
    }
}

#endif /* CATPAWS_CPCORE_CPMALLOC_H_ */
