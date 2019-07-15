#ifndef CATPAWS_CPCORE_CPPROXYALLOCATOR_H_
#define CATPAWS_CPCORE_CPPROXYALLOCATOR_H_

#include "CPAllocator.h"

class CPProxyAllocator : public CPAllocator
{
public:
    CPProxyAllocator(CPAllocator& allocator);
    virtual ~CPProxyAllocator();

    virtual void* Allocate(std::size_t size, uint8_t alignment = 4) override;
    virtual void Deallocate(void* p) override;

private:
    // Prevent Copy
    CPProxyAllocator(const CPProxyAllocator&) = delete;
    CPProxyAllocator& operator=(const CPProxyAllocator&) = delete;

    CPAllocator& allocator_;
};

#endif
