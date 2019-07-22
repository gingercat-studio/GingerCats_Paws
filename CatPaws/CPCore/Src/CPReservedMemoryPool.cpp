#include "CPCore.h"

CPReservedMemoryPool::CPReservedMemoryPool()
    : CPAllocator(0, nullptr)
{

}

CPReservedMemoryPool::~CPReservedMemoryPool()
{
    std::size_t backup_used_mem = used_memory_;
    std::size_t remain_memory = 0;
    std::size_t remain_allocation = 0;
    for (auto pair : memorypools_)
    {
        auto pool = pair.second.pool_;
        remain_memory += pool->UsedMemory();
        remain_allocation += pool->NumAllocation();
    }
    used_memory_ = remain_memory;
    num_allocations_ = remain_allocation;
}

void CPReservedMemoryPool::ReservePool()
{
    for (std::size_t i = 3, e = 13; i < e; ++i)
    {
        std::size_t object_size = (1LL << i);
        std::size_t pre_allocated_memory_size = object_size * 100
            + sizeof(CPPoolAllocator);

        for (std::size_t align = 2, maxalign = 8; align < maxalign; ++align)
        {
            uint8_t alignment = 1 << align;
            assert(alignment <= UINT8_MAX);
            void* malloc_memory = std::malloc(pre_allocated_memory_size);

            auto pool = new(malloc_memory) CPPoolAllocator
            (object_size, alignment,
                pre_allocated_memory_size - sizeof(CPPoolAllocator),
                PtrMath::Move
                (malloc_memory, sizeof(CPPoolAllocator)));

            CPMemKey key{ object_size, alignment };
            memorypools_[key] = CPPoolAllocatorNode{ pool, nullptr };

            size_ += pre_allocated_memory_size;
        }
    }
}

void CPReservedMemoryPool::ClearPool()
{
    for (auto pair : pooldictionary_)
    {
        pair.second->Deallocate(pair.first);
    }
}

void* CPReservedMemoryPool::Allocate(std::size_t size, uint8_t alignment)
{
    auto next = CPMath::ExpandToPowerOf2(size);
    auto node = memorypools_[{next, alignment}];

    // should check pool is full
    auto ptr = node.pool_->Allocate(next, alignment);
    pooldictionary_[ptr] = node.pool_;

    used_memory_ += node.pool_->ObjectSize();
    num_allocations_++;

    return ptr;
}

void CPReservedMemoryPool::Deallocate(void* p)
{
    auto pool = pooldictionary_[p];
    pooldictionary_.erase(p);
    pool->Deallocate(p);

    used_memory_ -= pool->ObjectSize();
    num_allocations_--;
}
