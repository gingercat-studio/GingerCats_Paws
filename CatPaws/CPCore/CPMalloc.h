#pragma once
class CPMalloc
{
public:
    CPMalloc() = delete;
    CPMalloc(size_t size, void* start)
    {

    }
    virtual ~CPMalloc();

protected:
    void* start_;
    size_t size_;
    size_t used_memory_;
    size_t num_allocators_;
};

