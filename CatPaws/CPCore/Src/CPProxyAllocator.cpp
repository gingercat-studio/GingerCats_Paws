#include "CPCore.h"

CPProxyAllocator::CPProxyAllocator(CPAllocator& allocator)
    : CPAllocator(allocator.Size(), allocator.Start())
    , allocator_(allocator)
{
}

CPProxyAllocator::~CPProxyAllocator()
{
}

void* CPProxyAllocator::Allocate(std::size_t size, uint8_t alignment)
{
    assert(size != 0);
    num_allocations_++;
    std::size_t mem_size = allocator_.UsedMemory();

    void* p = allocator_.Allocate(size, alignment);
    used_memory_ += allocator_.UsedMemory() - mem_size;

    return p;
}

void CPProxyAllocator::Deallocate(void* p)
{
    num_allocations_--;
    std::size_t mem_size = allocator_.UsedMemory();
    allocator_.Deallocate(p);
    used_memory_ = mem_size - allocator_.UsedMemory();
}
