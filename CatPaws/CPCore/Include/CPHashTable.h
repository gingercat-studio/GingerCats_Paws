#ifndef CATPAWS_CPCORE_CPHASHTABLE_H_
#define CATPAWS_CPCORE_CPHASHTABLE_H_

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

// change table size to prime number
struct CPPtrTableHash : public CPHash<uint64_t>
{
private:
    std::size_t table_size_ = 0;
public:
    CPPtrTableHash() = delete;
    CPPtrTableHash(std::size_t inTableSize) : table_size_(inTableSize) {};
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
    virtual std::size_t operator()(const uint64_t& key) const final
    {
        return (key / TableSize()) % TableSize();
    }
};

template<typename K, typename V
    , class HHash = CPPtrTableHash, class DHash = CPPtrTableHash2>
    class CPHashTable
{
private:
    const std::size_t element_size_ = sizeof(CPHashNode<K, V>);
    // upperbound = capacity + 1
    std::size_t capacity_ = 65535;
    std::size_t table_capacity_;
    std::size_t size_ = 0;

    CPHashNode<K, V>* htable_ = nullptr;
    CPHashNode<K, V>* dtable_ = nullptr;
    CPFreeListAllocator* alloc_ = nullptr;

    const std::size_t pre_allocated_mem_size_
        = (1 << 15) + sizeof(CPFreeListAllocator);

    HHash hhash_func_;
    DHash dhash_func_;

public:
    CPHashTable(std::size_t capacity = 65535) // 32kilo bytes
        : capacity_(capacity)
        , table_capacity_(capacity / 2)
        , hhash_func_(table_capacity_)
        , dhash_func_(table_capacity_)
    {
        auto pmem = std::malloc(pre_allocated_mem_size_);

        alloc_ = new (pmem)
            CPFreeListAllocator
            (pre_allocated_mem_size_ - sizeof(CPFreeListAllocator),
                PtrMath::Move
                (pmem,
                    sizeof(CPFreeListAllocator)));

        htable_ = Allocator::AllocateArray<CPHashNode<K, V>>(*alloc_, table_capacity_);
        dtable_ = Allocator::AllocateArray<CPHashNode<K, V>>(*alloc_, table_capacity_);
    }

    void Clear() noexcept
    {
        size_ = 0;
        std::memset(htable_, 0, sizeof(CPHashNode<K, V>) * table_capacity_);
        std::memset(dtable_, 0, sizeof(CPHashNode<K, V>) * table_capacity_);
    }

    bool HashInsert(const K& key, const V& value)
    {
        return HashInsert(key, value, htable_, dtable_);
    }

    bool HashInsert(const K& key, const V& value,
        CPHashNode<K, V>* htable, CPHashNode<K, V>* dtable)
    {
        auto hhash = hhash_func_(key);
        auto dhash = dhash_func_(key);

        if (htable[hhash].Key() == 0) // hash key should not be zero
        {
            htable[hhash] = CPHashNode<K, V>(key, value);
            return true;
        }

        if (dtable[dhash].Key() == 0)
        {
            // do cuckoo behavior
            dtable[dhash]
                = CPHashNode<K, V>(htable[hhash].Key(), htable[hhash].Value());

            htable[hhash] = CPHashNode<K, V>(key, value);
            return true;
        }

        // collision!
        return false;
    }


    void ReHash()
    {
        auto oldcapacity = capacity_;
        auto old_table_capacity = table_capacity_;
        CPHashNode<K, V>* oldhtable = htable_;
        CPHashNode<K, V>* olddtable = dtable_;

        bool collided = true;
        while (collided)
        {
            collided = false;
            table_capacity_ = capacity_;
            capacity_ = capacity_ * 2;

            hhash_func_.SetTableSize(table_capacity_);
            dhash_func_.SetTableSize(table_capacity_);

            CPHashNode<K, V>* htable
                = Allocator::AllocateArray<CPHashNode<K, V>>
                (*alloc_, table_capacity_);

            CPHashNode<K, V>* dtable
                = Allocator::AllocateArray<CPHashNode<K, V>>
                (*alloc_, table_capacity_);

            for (std::size_t i = 0; i < old_table_capacity; ++i)
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

            for (std::size_t i = 0; i < old_table_capacity; ++i)
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

        ++size_;
    }

    bool Remove(K key)
    {
        auto hhash = hhash_func_(key);
        if (htable_[hhash].Key() == key)
        {
            htable_[hhash] = CPHashNode<K, V>();
            --size_;
            return true;
        }

        auto dhash = dhash_func_(key);
        if (dtable_[dhash].Key() == key)
        {
            dtable_[dhash] = CPHashNode<K, V>();
            --size_;
            return true;
        }
        return false;
    }

    CPHashNode<K, V>* Find(K key)
    {
        auto hhash = hhash_func_(key);
        if (htable_[hhash].Key() == key)
        {
            return &htable_[hhash];
        }

        auto dhash = dhash_func_(key);
        if (dtable_[dhash].Key() == key)
        {
            return &dtable_[dhash];
        }

        return nullptr;
    }
};

#endif
