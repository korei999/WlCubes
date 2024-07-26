#pragma once

#include "Array.hh"
#include "hash.hh"

namespace adt
{

constexpr f64 HASHMAP_DEFAULT_LOAD_FACTOR = 0.5;

template<typename T>
struct Bucket
{
    T data;
    bool bOccupied = false;
    bool bDeleted = false;
};

template <typename T>
struct HashMapRet
{
    T* pData;
    size_t hash;
    u32 idx;
    bool bInserted;
};

/* `adt::fnHash<T>()` for hash function, linear probing */
template<typename T>
struct HashMap
{
    Allocator* pAlloc;
    Array<Bucket<T>> aBuckets;
    f64 maxLoadFactor;
    u32 bucketCount = 0;

    HashMap() = default;
    HashMap(Allocator* pAllocator) : pAlloc(pAllocator), aBuckets(pAllocator, SIZE_MIN), maxLoadFactor(HASHMAP_DEFAULT_LOAD_FACTOR) {}
    HashMap(Allocator* pAllocator, u32 prealloc) : pAlloc(pAllocator), aBuckets(pAllocator, prealloc), maxLoadFactor(HASHMAP_DEFAULT_LOAD_FACTOR) {}

    Bucket<T>& operator[](u32 i) { return this->aBuckets[i]; }
    const Bucket<T>& operator[](u32 i) const { return this->aBuckets[i]; }

    f64 loadFactor() const { return static_cast<f64>(this->bucketCount) / static_cast<f64>(this->aBuckets.capacity); }
    u32 capacity() const { return this->aBuckets.capacity; }
    HashMapRet<T> insert(const T& value);
    HashMapRet<T> search(const T& value);
    void remove(u32 i);
    void rehash(u32 _size);
    HashMapRet<T> tryInsert(const T& value);
    void destroy() { this->aBuckets.destroy(); }
};

template<typename T>
inline HashMapRet<T>
HashMap<T>::insert(const T& value)
{
    if (this->loadFactor() >= this->maxLoadFactor)
        this->rehash(this->capacity() * 2);

    size_t hash = fnHash(value);
    u32 idx = u32(hash % this->capacity());

    while (this->aBuckets[idx].bOccupied)
    {
        idx++;
        if (idx >= this->capacity())
            idx = 0;
    }

    this->aBuckets[idx].data = value;
    this->aBuckets[idx].bOccupied = true;
    this->aBuckets[idx].bDeleted = false;
    this->bucketCount++;

    return {
        .pData = &this->aBuckets[idx].data,
        .hash = hash,
        .idx = idx,
        .bInserted = true
    };
}

template<typename T>
inline HashMapRet<T>
HashMap<T>::search(const T& value)
{
    size_t hash = fnHash(value);
    u32 idx = u32(hash % this->capacity());

    HashMapRet<T> ret;
    ret.hash = hash;
    ret.pData = nullptr;
    ret.bInserted = false;

    while (this->aBuckets[idx].bOccupied || this->aBuckets[idx].bDeleted)
    {
        if (this->aBuckets[idx].data == value)
        {
            ret.pData = &this->aBuckets[idx].data;
            break;
        }

        idx++;
        if (idx >= this->capacity())
            idx = 0;
    }

    ret.idx = idx;
    return ret;
}

template<typename T>
inline void
HashMap<T>::remove(u32 i)
{
    this->aBuckets[i].bDeleted = true;
    this->aBuckets[i].bOccupied = false;
}

template<typename T>
inline void
HashMap<T>::rehash(u32 _size)
{
    auto mNew = HashMap<T>(this->aBuckets.pAlloc, _size);

    for (u32 i = 0; i < this->aBuckets.capacity; i++)
        if (this->aBuckets[i].bOccupied)
            mNew.insert(this->aBuckets[i].data);

    this->destroy();
    *this = mNew;
}

template<typename T>
inline HashMapRet<T>
HashMap<T>::tryInsert(const T& value)
{
    auto f = this->search(value);
    if (f.pData) return f;
    else return this->insert(value);
}

} /* namespace adt */
