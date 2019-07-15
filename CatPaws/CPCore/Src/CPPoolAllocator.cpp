#include "CPCore.h"

CPPoolAllocator::CPPoolAllocator(
    std::size_t object_size, uint8_t object_alignment, 
    std::size_t memory_size, void* mem)
    : CPAllocator(memory_size, mem)
    , object_alignment_(object_alignment)
    , object_size_(object_size)
{
    // object size should larger than pointer size
    // when they are free, block should store next pointer
    assert(object_size >= sizeof(void*)); 

    // keep align the object
    auto adjustment = PtrMath::AlignFowardAdjustment(mem, object_alignment);
    free_list_ = static_cast<void**>(PtrMath::Move(mem, adjustment));
    std::size_t num_objects = (memory_size - adjustment) / object_size_;
    auto p = free_list_;

    // initialize free block in the list;
    for (std::size_t i = 0; i < num_objects-1; ++i)
    {
        *p = PtrMath::Move(p, object_size);
        p = static_cast<void**>(*p);
    }

    *p = nullptr;
}

CPPoolAllocator::~CPPoolAllocator()
{
    free_list_ = nullptr;
}

void* CPPoolAllocator::Allocate(std::size_t size, uint8_t alignment)
{
    assert(size == object_size_ && alignment == object_alignment_);
    
    if (free_list_ == nullptr)
        return nullptr;

    void* p = free_list_;
    free_list_ = (void**)(*free_list_);
    used_memory_ += size;
    num_allocations_++;

    return p;
}

void CPPoolAllocator::Deallocate(void* p)
{
    *((void**)p) = free_list_;
    free_list_ = (void**)p;
    used_memory_ -= object_size_;
    num_allocations_--;
}
