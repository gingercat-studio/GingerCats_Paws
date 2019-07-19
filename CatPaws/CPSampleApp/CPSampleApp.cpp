// CPSampleApp.cpp : 이 파일에는 'main' 함수가 포함됩니다. 
// 거기서 프로그램 실행이 시작되고 종료됩니다.

// for benchmark and test
#include <iostream>
#include <vector> 
#include <algorithm>
#include <random>
#include <map>

#include "CPCore.h"

class TestClass
{
public:
    TestClass() = default;
   
private:
    int a = 0;
    int b = 1;
};

class TestClass2
{
public:
    TestClass2() = default;

private:
    long long a = 0;
    int b = 1;
    std::string string = "something woring";
};

class TestClass3
{
public:
    TestClass3() = default;

private:
    long long a = 0;
    int b = 1;
    std::string string1 = "something woring";
    std::string string2 = "something woring";
    std::string string3 = "something woring";
    std::string string4 = "something woring";
    std::string string5 = "something woring";
    std::string string6 = "something woring";
};

class CPTestMallocContainer
{
public:
    CPTestMallocContainer() = delete;
    CPTestMallocContainer(const CPTestMallocContainer&) = delete;
    CPTestMallocContainer& operator=(const CPTestMallocContainer&) = delete;

    CPTestMallocContainer(std::size_t demandedmemorysize)
        : allocated_memory_size_{demandedmemorysize}
    {
        allocated_memory_ = std::malloc(demandedmemorysize);
    }

    ~CPTestMallocContainer()
    {
        delete allocated_memory_;
    }

    void* AllocatedMemory() { return allocated_memory_; }
    std::size_t AllocatedMemorySize() { return allocated_memory_size_; }

private:
    void* allocated_memory_;
    std::size_t allocated_memory_size_;
};

class CPMemoryPools  : public CPAllocator// just for temp name
{
private:
    struct CPMemoryKey
    {
        std::size_t size_;
        uint8_t alignment_;
        bool operator<(const CPMemoryKey& other) const
        {
            return size_ < other.size_&& alignment_ < other.alignment_;
        }
    };

    struct CPPoolAllocatorNode
    {
        CPPoolAllocator* pool_ = nullptr;
        CPPoolAllocatorNode* next_ = nullptr;
    };

    std::map<CPMemoryKey, CPPoolAllocatorNode> memorypools_;
    std::map<void*, CPPoolAllocator*> pooldictionary_;

public:
    CPMemoryPools()
        : CPAllocator(0, nullptr)
    {
    
    }

    virtual ~CPMemoryPools()
    {
    
    }

    void ReservePool()
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

                CPMemoryKey key{ object_size, alignment };
                memorypools_[key] = CPPoolAllocatorNode{ pool, nullptr };

                size_ += pre_allocated_memory_size;
            }
        }        
    }
    // Pool Allocator with freeList allocator

    void ClearPool()
    {
        for (auto pair : memorypools_)
        {
            auto pool_node = pair.second;
            pool_node.pool_->~CPPoolAllocator();
       }
    }

    virtual void* Allocate(std::size_t size, uint8_t alignment = 4) override
    {
        auto node = memorypools_[{size, alignment}];
      
        // should check pool is full
        auto ptr = node.pool_->Allocate(size, alignment);
        pooldictionary_[ptr] = node.pool_;
        
        used_memory_ += node.pool_->ObjectSize();
        num_allocations_++;

        return ptr;
    }
    
    virtual void Deallocate(void* p) override
    {
        auto pool = pooldictionary_[p];
        pooldictionary_.erase(p);
        pool->Deallocate(p);

        used_memory_ -= pool->ObjectSize();
        num_allocations_--;
    }
};

void LinearAllocatorTest()
{
    CPTestMallocContainer temp_container(1024*1024);
    std::size_t pre_allocated_memory_size = temp_container.AllocatedMemorySize();
    
    auto somelinearallocator =
        new (temp_container.AllocatedMemory())
        CPLinearAllocator(pre_allocated_memory_size - sizeof(CPLinearAllocator),
            PtrMath::Move
            (temp_container.AllocatedMemory(), sizeof(CPLinearAllocator)));

    int count = 0;
    std::vector<TestClass*> ptr_container;

    while (true)
    {
        auto testinst = Allocator::AllocateNew<TestClass>(*somelinearallocator);
        if (somelinearallocator->Size() - somelinearallocator->UsedMemory()
            < sizeof(TestClass))
        {
            std::cout << "Linear Allocator Full!\n";
            break;
        }
        ++count;
        ptr_container.push_back(testinst);
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(ptr_container.begin(), ptr_container.end(), g);

    for (auto ptr : ptr_container)
    {
        //Allocator::DeallocateDelete(*somelinearallocator, ptr);
        somelinearallocator->Clear();
    }

    std::cout << "Linear Allocator Full!\n";
}

void StackAllocatorTest()
{
    CPTestMallocContainer temp_container(1024 * 1024);
    std::size_t pre_allocated_memory_size = temp_container.AllocatedMemorySize();
    assert(pre_allocated_memory_size > sizeof(CPStackAllocator));

    auto somestackallocator =
        new (temp_container.AllocatedMemory())
        CPStackAllocator(pre_allocated_memory_size - sizeof(CPStackAllocator),
            PtrMath::Move
            (temp_container.AllocatedMemory(), sizeof(CPStackAllocator)));
    int count = 0;
    std::vector<TestClass*> ptr_container;

    while (true)
    {
        auto testinst = Allocator::AllocateNew<TestClass>(*somestackallocator);
        ++count;
        ptr_container.push_back(testinst);

        // The "real" stack size is sizeof(someclass) + stack header size
        if (somestackallocator->Size() > somestackallocator->UsedMemory()
            && somestackallocator->Size() - somestackallocator->UsedMemory()
            < (sizeof(TestClass) + somestackallocator->SizeofHeader()))
        {
            std::cout << "Stack Allocator Full!\n";
            break;
        }
    }

    // stack allocator is not free access memory allocator.
    // use FILO Strategy
    //std::random_device rd;
    //std::mt19937 g(rd());
    //std::shuffle(ptr_container.begin(), ptr_container.end(), g);
    while (auto last = ptr_container.size())
    {
        auto ptr = ptr_container[last-1];
        Allocator::DeallocateDelete(*somestackallocator, *ptr);
        ptr_container.pop_back();
    }

    std::cout << "Stack Allocator Deallocated!\n";

    //for (auto ptr : ptr_container)
    //{
    //    Allocator::DeallocateDelete(*somestackallocator, ptr);
    //}
}

void FreeListAllocatorTest()
{
    CPTestMallocContainer temp_container(1024 * 1024);
    std::size_t pre_allocated_memory_size = temp_container.AllocatedMemorySize();

    auto somefreelistallocator =
        new (temp_container.AllocatedMemory())
        CPFreeListAllocator
        (pre_allocated_memory_size - sizeof(CPFreeListAllocator),
            PtrMath::Move
            (temp_container.AllocatedMemory(), sizeof(CPFreeListAllocator)));

    int count = 0;
    std::vector<TestClass*> ptr_container;

    while (true)
    {
        auto testinst = Allocator::AllocateNew<TestClass>(*somefreelistallocator);
        // is this stuff right? 
        if (somefreelistallocator->Size() - somefreelistallocator->UsedMemory()
            < (sizeof(TestClass) 
                + somefreelistallocator->SizeofBlock() 
                + somefreelistallocator->SizeofHeader()))
        {
            std::cout << "FreeList Allocator Full!\n";
            break;
        }

        ptr_container.push_back(testinst);
        ++count;
    }


    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(ptr_container.begin(), ptr_container.end(), g);

    for (auto ptr : ptr_container)
    {
        Allocator::DeallocateDelete(*somefreelistallocator, *ptr);
    }

    std::cout << "FreeList Allocator Deallocated!\n";
}

void PoolAllocatorTest()
{
    CPTestMallocContainer temp_container(1024 * 1024);
    std::size_t pre_allocated_memory_size = temp_container.AllocatedMemorySize();

    auto somepoolallocator =
        new (temp_container.AllocatedMemory())
        CPPoolAllocator
        (   sizeof(TestClass), alignof(TestClass),
            pre_allocated_memory_size - sizeof(CPPoolAllocator),
            PtrMath::Move
            (temp_container.AllocatedMemory(), sizeof(CPPoolAllocator)));

    int count = 0;
    
    std::vector<TestClass*> ptr_container;
    while (true)
    {
        auto testinst = Allocator::AllocateNew<TestClass>(*somepoolallocator);
        // is this stuff right? 
        if (somepoolallocator->Size() - somepoolallocator->UsedMemory()
            < (sizeof(TestClass)))
        {
            std::cout << "Pool Allocator Full!\n";
            break;
        }
        ptr_container.push_back(testinst);
        ++count;
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(ptr_container.begin(), ptr_container.end(), g);

    for (auto ptr : ptr_container)
    {
        Allocator::DeallocateDelete(*somepoolallocator, *ptr);
    }

    std::cout << "Pool Allocator Deallocated!\n";
}


int main()
{
    //LinearAllocatorTest();
    //StackAllocatorTest();
    //FreeListAllocatorTest();
    //PoolAllocatorTest();

    // WIP
    //CPMemoryPools t;
    //t.ReservePool();
    //for (int i = 0, e = 100; i < e; ++i)
    //{
    //    auto testinst = Allocator::AllocateNew<TestClass>(t);
    //}
    //
    //for (int i = 0, e = 100; i < e; ++i)
    //{
    //    auto testinst2 = Allocator::AllocateNew<TestClass2>(t);
    //}
    //
    //for (int i = 0, e = 100; i < e; ++i)
    //{
    //    auto testinst3 = Allocator::AllocateNew<TestClass3>(t);
    //}

    std::cout << "Memory Allocator Test Done!\n"; 
}

// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, 
//      [프로젝트] > [기존 항목 추가]로 이동하여
//      기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고
//       .sln 파일을 선택합니다.
