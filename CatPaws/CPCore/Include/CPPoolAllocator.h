#ifndef CATPAWS_CPCORE_CPPOOLALLOCATOR_H_
#define CATPAWS_CPCORE_CPPOOLALLOCATOR_H_

#include "CPAllocator.h"

class CPPoolAllocator : public CPAllocator
{
public:
    CPPoolAllocator(std::size_t object_size, uint8_t object_alignment,
        std::size_t memory_size, void* mem);
    virtual ~CPPoolAllocator();

    // Inherited via CPAllocator
    virtual void* Allocate(std::size_t size, uint8_t alignment = 4) override;
    virtual void Deallocate(void* p) override;

    auto ObjectSize() { return object_size_; }

private:
    // Prevent Copy
    CPPoolAllocator(const CPPoolAllocator&) = delete;
    CPPoolAllocator& operator=(const CPPoolAllocator&) = delete;

    std::size_t object_size_;
    uint8_t object_alignment_;
    void** free_list_;
};

#endif
