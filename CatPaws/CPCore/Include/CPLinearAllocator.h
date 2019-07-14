#ifndef CATPAWS_CPCORE_CPLINEARALLOCATOR_H_
#define CATPAWS_CPCORE_CPLINEARALLOCATOR_H_

#include "CPAllocator.h"

class CPLinearAllocator : public CPAllocator
{
public:
    CPLinearAllocator(std::size_t size, void* start);
    virtual ~CPLinearAllocator();

    virtual void* Allocate(std::size_t size, uint8_t alignment = 4) override;
    virtual void Deallocate(void* p) override;
    void Clear();

private:
    // Prevent Copy
    CPLinearAllocator(const CPLinearAllocator&) = delete;
    CPLinearAllocator& operator=(const CPLinearAllocator&) = delete;

    // variables
    void* current_pos_;
};


#endif
