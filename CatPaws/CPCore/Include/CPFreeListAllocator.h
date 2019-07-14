#ifndef CATPAWS_CPCORE_CPFREELISTALLOCATOR_H_
#define CATPAWS_CPCORE_CPFREELISTALLOCATOR_H_

#include "CPAllocator.h"

class CPFreeListAllocator : public CPAllocator
{
public:
    CPFreeListAllocator(std::size_t size, void* start);
    virtual ~CPFreeListAllocator();

    // Inherited via CPAllocator
    virtual void* Allocate(std::size_t size, uint8_t alignment = 4) override;
    virtual void Deallocate(void* p) override;
    
    constexpr std::size_t SizeofHeader() { 
        return sizeof(CPFreeListAllocationHeader); }

    constexpr std::size_t SizeofBlock() { return sizeof(CPFreeListFreeBlock); }

private:
    // Prevent Copy
    CPFreeListAllocator(const CPFreeListAllocator&) = delete;
    CPFreeListAllocator& operator=(const CPFreeListAllocator&) = delete;

    struct CPFreeListAllocationHeader
    {
        std::size_t size_;
        uintptr_t adjustment_;
    };

    struct CPFreeListFreeBlock
    {
        std::size_t size_;
        CPFreeListFreeBlock* next_;
    };

    CPFreeListFreeBlock* free_blocks_;
};

#endif
