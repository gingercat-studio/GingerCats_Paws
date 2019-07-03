#ifndef CATPAWS_CPCORE_CPMALLOC_H_
#define CATPAWS_CPCORE_CPMALLOC_H_

//Before do real thing, I just followed Tiago Costa's GameDev Tutorial
//https://www.gamedev.net/articles/programming/general-and-gameplay-programming/c-custom-memory-allocation-r3010/

namespace PtrMath{
    inline void* AlignFoward(void* address, uint8_t alignment)
    {
        return (void*)
            ((reinterpret_cast<uintptr_t>(address) +
                static_cast<uintptr_t>(alignment - 1)) &
                static_cast<uintptr_t>(~(alignment - 1)));
    }

    inline uintptr_t AlignFowardAdjustment(void* address, uint8_t alignment)
    {
        auto adjustment = alignment -
            (reinterpret_cast<uintptr_t>(address) &
                static_cast<uintptr_t>(alignment - 1));

        if (adjustment == alignment) return 0;
        return adjustment;
    }

    inline uintptr_t AlignFowardAdjustmentWithHeader(
        void* address, uint8_t alignment, uint8_t headersize)
    {
        auto adjustment = AlignFowardAdjustment(address, alignment);
        uintptr_t neededspace = headersize;

        if (adjustment < neededspace)
        {
            neededspace -= adjustment;
            adjustment += static_cast<uint8_t>(alignment) * 
                static_cast<uint8_t>((neededspace / alignment));
            if ((neededspace % alignment) > 0)
            {
                adjustment += alignment;
            }
        }

        return adjustment;
    }
}

class CPAllocator
{
public:
    CPAllocator() = delete;

    CPAllocator(size_t size, void* start)
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

    virtual void* Allocate(size_t size, uint8_t alignment = 4) = 0;
    virtual void Deallocate(void* p) = 0;
    void* Start() const { return start_; }
    size_t Size() const { return size_; }
    size_t UsedMemory() const { return used_memory_; }
    size_t NumAllocation() const { return num_allocations_; }

protected:
    void* start_;
    size_t size_;
    size_t used_memory_;
    size_t num_allocations_;
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
