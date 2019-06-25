#ifndef CATPAWS_CPCORE_CPMALLOC_H_
#define CATPAWS_CPCORE_CPMALLOC_H_

class CPAllocator
{
public:
    CPAllocator() = delete;

    CPAllocator(size_t size, void* start)
        : start_{ start }, 
        size_{ size }, 
        used_memory_ { 0 }, 
        num_allocators_ { 0 }
    {}

    virtual ~CPAllocator()
    {
        assert(num_allocators_ == 0 && used_memory_ == 0);
        start_ = nullptr;
        size_ = 0;
    }

    virtual void* Allocate(size_t size, uint8_t alignment = 4) = 0;
    virtual void Deallocate(void* p) = 0;
    void* Start() const { return start_; }
    size_t Size() const { return size_; }
    size_t UsedMemory() const { return used_memory_; }
    size_t NumAllocation() const { return num_allocators_; }

protected:
    void* start_;
    size_t size_;
    size_t used_memory_;
    size_t num_allocators_;
};

namespace allocator
{
    template<class T> T* AllocateNew(CPAllocator& allocator, size_t length)
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

    template<class T> T* AllocateArray(CPAllocator& allocator, size_t length)
    {
        assert(length != 0);
        uint8_t headersize = sizeof(size_t) / sizeof(T);

        if (sizeof(size_t) & sizeof(T) > 0)
        {
            ++headersize;
        }

        T* p = ((T*)allocator.Allocate(sizeof(T) * 
            (length + headersize), alignof(T))) + headersize;

        *(((size_t*)p)--) = length;

        for (size_t i = 0; i < length; i++)
        {
            new (&p) T;
        }

        return p;
    }

    template <class T> void DeallocateArray(CPAllocator& allocator, T* array)
    {
        assert(array != nullptr);
        size_t length = *(((size_t*)array)--);

        for (size_t i = 0; i < length; i++)
        {
            array.~T();
        }

        uint8_t headersize = sizeof(size_t) / sizeof(T);

        if (sizeof(size_t) % sizeof(T) > 0)
        {
            headersize++;
        }

        allocator.Deallocate(array - headersize);
    }
}

#endif /* CATPAWS_CPCORE_CPMALLOC_H_ */
