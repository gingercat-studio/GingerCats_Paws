// CPSampleApp.cpp : 이 파일에는 'main' 함수가 포함됩니다. 
// 거기서 프로그램 실행이 시작되고 종료됩니다.

// for benchmark and test
#include <iostream>
#include <vector> 
#include <algorithm>
#include <random>

#include "CPCore.h"


class BaseTestClass
{
public:
    BaseTestClass() = default;
};

class TestClass : public BaseTestClass
{
public:
    TestClass() = default;
   
private:
    int a = 0;
    int b = 1;
};

class TestClass2 : public BaseTestClass
{
public:
    TestClass2() = default;

private:
    long long a = 0;
    int b = 1;
    std::string string = "something wrong";
};

class TestClass3 : public BaseTestClass
{
public:
    TestClass3() = default;

private:
    long long a = 0;
    int b = 1;
    std::string string1 = "something wrong";
    std::string string2 = "something wrong";
    std::string string3 = "something wrong";
    std::string string4 = "something wrong";
    std::string string5 = "something wrong";
    std::string string6 = "something wrong";
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

void ProtoTypePoolAllocatorTest()
{
    std::vector<BaseTestClass*> alloced_instances;
    alloced_instances.reserve(300);

    CPReservedMemoryPool t;
    t.ReservePool();
    for (int i = 0, e = 100; i < e; ++i)
    {
        auto testinst = Allocator::AllocateNew<TestClass>(t);
        assert(testinst != nullptr);
        alloced_instances.push_back(testinst);
    }

    for (int i = 0, e = 100; i < e; ++i)
    {
        auto testinst2 = Allocator::AllocateNew<TestClass2>(t);
        assert(testinst2 != nullptr);
        alloced_instances.push_back(testinst2);
    }

    for (int i = 0, e = 80; i < e; ++i)
    {
        auto testinst3 = Allocator::AllocateNew<TestClass3>(t);
        assert(testinst3 != nullptr);
        alloced_instances.push_back(testinst3);
    }

    auto arraycheck = Allocator::AllocateArray<TestClass3>(t, 10);

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(alloced_instances.begin(), alloced_instances.end(), g);

    for (auto ptr : alloced_instances)
    {
        assert(ptr != nullptr);
        Allocator::DeallocateDelete(t, *ptr);
    }

    Allocator::DeallocateArray(t, arraycheck);
    arraycheck = nullptr;


    t.ClearPool();
}

template<typename K, typename V>
class CPHashNode
{
private:
    K key_;
    V value_;
public:
    CPHashNode() : key_{ 0 }, value_{ 0 } {}
    CPHashNode(K key, V value) : key_{ key }, value_{ value } {}
    K& Key() { return key_; }
    V& Value() { return value_; }
};

template<class Key>
struct CPHash
{
public:
    CPHash() = default;
    std::size_t operator()(const Key& key) const
    {
        return 0;
    }
};

/*  Written in 2015 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */
template<> 
struct CPHash<uint64_t>
{
public:
    CPHash() = default;
    virtual std::size_t operator()(const uint64_t& key) const
    {
        auto x = key;
        uint64_t z = (x += 0x9e3779b97f4a7c15);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        return z ^ (z >> 31);
    }
};

struct CPPtrTableHash : public CPHash<uint64_t>
{
private:
    std::size_t table_size_ = 0;
public:
    CPPtrTableHash() = delete;
    CPPtrTableHash(std::size_t inTableSize) : table_size_(inTableSize) {} ;
    virtual std::size_t operator()(const uint64_t& key) const
    {
        return key % table_size_;
    }
    
    const auto TableSize() const { return table_size_; }
    void SetTableSize(const std::size_t& new_size) { table_size_ = new_size; }
};

struct CPPtrTableHash2 : public CPPtrTableHash
{
public:
    CPPtrTableHash2(std::size_t inTableSize) : CPPtrTableHash(inTableSize) {};
    virtual std::size_t operator()(const uint64_t& key) const
    {
        return (key / TableSize()) % TableSize();
    }
};

template<typename K, typename V>
class CPUnorderedMap
{
private:
    // upperbound = capacity + 1
    std::size_t capacity_ = 64;
    std::size_t size_ = 0;

    CPHashNode<K, V>* htable_ = nullptr;
    CPHashNode<K, V>* dtable_ = nullptr;
    CPFreeListAllocator* alloc_ = nullptr;

    const std::size_t pre_allocated_mem_size = 1 << 26 ;

    CPTestMallocContainer temp_malloc_container_;

    CPPtrTableHash hash_func_;
    CPPtrTableHash2 hash_func2_;

public:
    CPUnorderedMap(std::size_t capacity = 1 << 20)
        : temp_malloc_container_(pre_allocated_mem_size)
        , capacity_(capacity)
        , hash_func_(capacity_)
        , hash_func2_(capacity_)
    {
        std::size_t pre_allocated_memory_size
            = temp_malloc_container_.AllocatedMemorySize();
            
        alloc_ = new (temp_malloc_container_.AllocatedMemory())
            CPFreeListAllocator
            (pre_allocated_memory_size - sizeof(CPFreeListAllocator),
                PtrMath::Move
                (temp_malloc_container_.AllocatedMemory(), 
                    sizeof(CPFreeListAllocator)));
        
        htable_ = Allocator::AllocateArray<CPHashNode<K, V>>(*alloc_, capacity_);
        dtable_ = Allocator::AllocateArray<CPHashNode<K, V>>(*alloc_, capacity_);
    }

    void Clear() noexcept
    {
        size_ = 0;
        std::memset(htable_, 0, sizeof(CPHashNode<K, V>) * capacity_);
        std::memset(dtable_, 0, sizeof(CPHashNode<K, V>) * capacity_);
    }

    bool HashInsert(K& key, V& value)
    {
        return HashInsert(key, value, htable_, dtable_);
    }

    bool HashInsert(K& key, V& value, 
        CPHashNode<K,V>* htable, CPHashNode<K,V>* dtable)
    {
        auto hash1 = hash_func_(key);
        auto hash2 = hash_func2_(key);

        if (htable[hash1].Key() == 0) // hash key should not be zero
        {
            htable[hash1] = CPHashNode<K, V>(key, value);
            return true;
        }
        
        if (dtable[hash2].Key() == 0)
        {
            // do cuckoo behavior
            dtable[hash2] 
                = CPHashNode<K,V>(htable[hash1].Key(), htable[hash1].Value());
            
            htable[hash1] = CPHashNode<K, V>(key, value);
            return true;
        }

        // collision!
        return false;
    }


    void ReHash()
    {
        auto oldcapacity = capacity_;
        CPHashNode<K, V>* oldhtable = htable_;
        CPHashNode<K, V>* olddtable = dtable_;

        bool collided = true;
        while (collided)
        {
            collided = false;
            capacity_ = capacity_ << 1;

            hash_func_.SetTableSize(capacity_);
            hash_func2_.SetTableSize(capacity_);

            CPHashNode<K, V>* htable
                = Allocator::AllocateArray<CPHashNode<K, V>>(*alloc_, capacity_);

            CPHashNode<K, V>* dtable
                = Allocator::AllocateArray<CPHashNode<K, V>>(*alloc_, capacity_);

            for (std::size_t i = 0; i < oldcapacity; ++i)
            {
                if (collided) break;
                if (oldhtable[i].Key() != 0)
                {
                    collided = HashInsert(oldhtable[i].Key()
                        , oldhtable[i].Value()
                        , htable
                        , dtable) == false;
                }
            }

            for (std::size_t i = 0; i < oldcapacity; ++i)
            {
                if (collided) break;
                if (olddtable[i].Key() != 0)
                {
                    collided = HashInsert(olddtable[i].Key()
                        , olddtable[i].Value()
                        , htable
                        , dtable) == false;
                }
            }

            if (collided)
            {
                Allocator::DeallocateArray(*alloc_, htable);
                Allocator::DeallocateArray(*alloc_, dtable);
            }
            else
            {
                htable_ = htable;
                dtable_ = dtable;
            }
        }

        Allocator::DeallocateArray(*alloc_, oldhtable);
        Allocator::DeallocateArray(*alloc_, olddtable);
    }

    void Insert(K key, V value)
    {
        if (HashInsert(key, value) == false)
        {
            ReHash();            
            HashInsert(key, value);
        }
    }

    CPHashNode<K,V>* Find(K key)
    {
        auto hash1 = hash_func_(key);
        if (htable_[hash1].Key() == key)
        {
            return htable_[hash1];
        }
            
        auto hash2 = hash_func2_(key);
        if (dtable_[hash2].Key() == key)
        {
            return dtable_[hash2];
        }

        return nullptr;
    }
};


int main()
{
    //LinearAllocatorTest();
    //StackAllocatorTest();
    //FreeListAllocatorTest();
    //PoolAllocatorTest();
    //ProtoTypePoolAllocatorTest();
    CPUnorderedMap<uintptr_t, uintptr_t> Test;
    for (std::size_t i = 0; i < 65535; ++i)
    {
        Test.Insert(i, i*100);
    }

    Test.Clear();

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
