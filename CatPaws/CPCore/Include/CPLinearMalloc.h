#ifndef CATPAWS_CPCORE_CPLINEARMALLOC_H_
#define CATPAWS_CPCORE_CPLINEARMALLOC_H_

#include "CPMalloc.h"

class CPLinearAllocator : public CPAllocator
{
public:
    CPLinearAllocator(size_t size, void* start);
    virtual ~CPLinearAllocator();

    virtual void* Allocate(size_t size, uint8_t alignment = 4);
    virtual void Deallocate(void* p);
    void Clear();

private:
    // Prevent Copy
    CPLinearAllocator(const CPLinearAllocator&) = delete;
    CPLinearAllocator& operator=(const CPLinearAllocator&) = delete;

    // variables
    void* current_pos_;
};


#endif
