#ifndef CATPAWS_CPCORE_CPRESERVEDMEMORYPOOL_H_
#define CATPAWS_CPCORE_CPRESERVEDMEMORYPOOL_H_

struct CPMemKey
{
    std::size_t size_;
    uint8_t alignment_;
    bool operator==(const CPMemKey& other) const
    {
        return (size_ == other.size_
            && alignment_ == other.alignment_);
    }
};

class CPMemKeyHashFunction
{
public:
    size_t operator()(const CPMemKey& p) const
    {
        return p.size_^ p.alignment_;
    }
};

struct CPPoolAllocatorNode
{
    CPPoolAllocator* pool_ = nullptr;
    CPPoolAllocatorNode* next_ = nullptr;
};

class CPReservedMemoryPool : public CPAllocator// just for temp name
{
public:
    CPReservedMemoryPool();
    virtual ~CPReservedMemoryPool();

    void ReservePool();
    void ClearPool();

    virtual void* Allocate(std::size_t size, uint8_t alignment = 4) override;
    virtual void Deallocate(void* p) override;

private:
    // @todo arkiny remove stl from the header!
    std::unordered_map
        <CPMemKey, CPPoolAllocatorNode, CPMemKeyHashFunction> memorypools_;
    std::unordered_map<void*, CPPoolAllocator*> pooldictionary_;
};

#endif
