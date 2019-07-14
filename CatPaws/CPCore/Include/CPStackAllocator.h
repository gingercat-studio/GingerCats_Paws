#ifndef CATPAWS_CPCORE_CPSTACKALLOCATOR_H_
#define CATPAWS_CPCORE_CPSTACKALLOCATOR_H_

#include "CPAllocator.h"

class CPStackAllocator : public CPAllocator
{
public:
    CPStackAllocator(std::size_t size, void* start);
    virtual ~CPStackAllocator();

    virtual void* Allocate(std::size_t size, uint8_t alignment = 4) override;
    virtual void Deallocate(void* p) override;
    constexpr std::size_t SizeofHeader() { return sizeof(CPAllocationHeader); }

private:
    // Prevent Copy
    CPStackAllocator(const CPStackAllocator&) = delete;
    CPStackAllocator& operator=(const CPStackAllocator&) = delete;

    struct CPAllocationHeader
    {
#if _DEBUG
        void* prev_address_;
#endif
        uintptr_t adjustment_;
    };

#if _DEBUG
    void* prev_position_;
#endif

    void* current_position_;
};

#endif
