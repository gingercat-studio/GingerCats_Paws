#include "CPCore.h"

CPFreeListAllocator::CPFreeListAllocator(std::size_t size, void* start)
    : CPAllocator(size, start)
    , free_blocks_(reinterpret_cast<CPFreeListFreeBlock*>(start))
{
    assert(size > sizeof(CPFreeListFreeBlock));
    free_blocks_->size_ = size;
    free_blocks_->next_ = nullptr;
}

CPFreeListAllocator::~CPFreeListAllocator()
{
    free_blocks_ = nullptr;
}

void* CPFreeListAllocator::Allocate(std::size_t size, uint8_t alignment)
{
    assert(size != 0 && alignment != 0);
    CPFreeListFreeBlock* prev_free_block = nullptr;
    CPFreeListFreeBlock* free_block = free_blocks_;

    while (free_block != nullptr)
    {
        // Calc adjusted aligned size;
        auto adjustment = PtrMath::AlignFowardAdjustmentWithHeader
                    (free_block, alignment, sizeof(CPFreeListAllocationHeader));
        auto totalsize = size + adjustment;

        // if allocation doesn't fit in this freeblock, find next
        if (free_block->size_ < totalsize)
        {
            prev_free_block = free_block;
            free_block = free_block->next_;
            continue;
        }

        static_assert
            (sizeof(CPFreeListAllocationHeader) >= sizeof(CPFreeListFreeBlock),
            "sizeof(CPFreeListAllocationHeader) < sizeof(CPFreeListFreeBlock)");
    
        if (free_block->size_ - totalsize <= sizeof(CPFreeListAllocationHeader))
        {
            totalsize = free_blocks_->size_;

            if (prev_free_block != nullptr)
            {
                prev_free_block->next_ = free_blocks_->next_;
            }
            else
            {
                free_blocks_ = free_block->next_;
            }
        }
        else
        {
            auto next_block = (CPFreeListFreeBlock*)
                (PtrMath::Move(free_block, totalsize));

            next_block->size_ = free_block->size_ - totalsize;
            next_block->next_ = free_block->next_;

            if (prev_free_block != nullptr)
            {
                prev_free_block->next_ = next_block;
            }
            else
            {
                free_blocks_ = next_block;
            }
        }

        auto aligned_address = (uintptr_t)free_block + adjustment;
        auto header = (CPFreeListAllocationHeader*)
            (aligned_address - sizeof(CPFreeListAllocationHeader));
        
        header->size_ = totalsize;
        header->adjustment_ = adjustment;
        
        used_memory_ += totalsize;
        num_allocations_++;
        
        assert(PtrMath::AlignFowardAdjustment(
            (void*)aligned_address, alignment) == 0);

        return (void*)aligned_address;
    }

    return nullptr;
}

void CPFreeListAllocator::Deallocate(void* p)
{
    assert(p != nullptr);
    
    auto header =
        (CPFreeListAllocationHeader*)
        PtrMath::Move
        (p, -static_cast<int64_t>(sizeof(CPFreeListAllocationHeader)));

    uintptr_t block_start 
        = reinterpret_cast<uintptr_t>(p) - header->adjustment_;

    auto block_size = header->size_;
    auto block_end = block_start + block_size;

    CPFreeListFreeBlock* prev_free_block = nullptr;
    CPFreeListFreeBlock* free_block = free_blocks_;

    while (free_block != nullptr)
    {
        if ((uintptr_t)free_block >= block_end)
            break;
        prev_free_block = free_block;
        free_block = free_block->next_;
    }

    if (prev_free_block == nullptr)
    {
        prev_free_block = (CPFreeListFreeBlock*)block_start;
        prev_free_block->size_ = block_size;
        prev_free_block->next_ = free_blocks_;
        free_blocks_ = prev_free_block;
    }
    else if ((uintptr_t)prev_free_block + prev_free_block->size_ == block_start)
    {
        prev_free_block->size_ += block_size;
    }
    else
    {
        auto temp = (CPFreeListFreeBlock*)block_start;
        temp->size_ = block_size;
        temp->next_ = prev_free_block->next_;
        prev_free_block->next_ = temp;
        prev_free_block = temp;
    }

    if (free_block != nullptr && (uintptr_t)free_block == block_end)
    {
        prev_free_block->size_ += free_block->size_;
        prev_free_block->next_ = free_block->next_;
    }

    num_allocations_--;
    used_memory_ -= block_size;
}
