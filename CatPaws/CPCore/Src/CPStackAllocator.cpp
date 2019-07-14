#include "CPCore.h"

CPStackAllocator::CPStackAllocator(std::size_t size, void* start)
    :CPAllocator(size, start)
    , current_position_{start}
{
    assert(size > 0);
#if _DEBUG
    prev_position_ = nullptr;
#endif
}

CPStackAllocator::~CPStackAllocator()
{
#if _DEBUG
    prev_position_ = nullptr;
#endif

    current_position_ = nullptr;
}

void* CPStackAllocator::Allocate(std::size_t size, uint8_t alignment)
{
    assert(size != 0);
    
    uintptr_t adjustment = 
        PtrMath::AlignFowardAdjustmentWithHeader
        (current_position_, alignment, sizeof(CPAllocationHeader));
    
    if (used_memory_ + adjustment + size > size_)
        return nullptr;

    void* aligned_address = PtrMath::Move(current_position_, adjustment);

    CPAllocationHeader* header = (CPAllocationHeader*)
        (PtrMath::Move(aligned_address, 
            -static_cast<int64_t>(sizeof(CPAllocationHeader))));
    header->adjustment_ = adjustment;

#if _DEBUG
    header->prev_address_ = prev_position_;
    prev_position_ = aligned_address;
#endif

    current_position_ = PtrMath::Move(aligned_address, size);
    used_memory_ += size + adjustment;
    num_allocations_++;

    return aligned_address;
}

void CPStackAllocator::Deallocate(void* p)
{
#if _DEBUG
    assert(p == prev_position_);
#endif
    // Access the allocation header
    CPAllocationHeader* header = (CPAllocationHeader*)
        (PtrMath::Move(p, 
            -static_cast<int64_t>(sizeof(CPAllocationHeader))));

    used_memory_ -= (uintptr_t)current_position_ 
        - (uintptr_t)p + header->adjustment_;

    current_position_ = PtrMath::Move(p, 
        -static_cast<int64_t>((header->adjustment_)));

#if _DEBUG
    prev_position_ = header->prev_address_;
#endif

    num_allocations_--;
}
