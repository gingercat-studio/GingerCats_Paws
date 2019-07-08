#include "CPCore.h"

CPLinearAllocator::CPLinearAllocator(std::size_t size, void* start)
    : CPAllocator{ size, start },
    current_pos_{start}
{
    assert(size > 0);
}

CPLinearAllocator::~CPLinearAllocator()
{
    current_pos_ = nullptr;
}

void* CPLinearAllocator::Allocate(std::size_t size, uint8_t alignment)
{
    assert(size != 0);
    uintptr_t adjustment =
        PtrMath::AlignFowardAdjustment(current_pos_, alignment);

    if (used_memory_ + adjustment + size > size_)
        return nullptr;

    uintptr_t aligned_address = 
        reinterpret_cast<uintptr_t>(current_pos_) + adjustment;

    current_pos_ = (void*)(aligned_address + size);
    used_memory_ += size + adjustment;
    num_allocations_++;

    return (void*)aligned_address;
}

void CPLinearAllocator::Deallocate(void* p)
{
    assert(false && "Use Clear() Instead");
}

void CPLinearAllocator::Clear()
{
    num_allocations_ = 0;
    used_memory_ = 0;
    current_pos_ = start_;
}
