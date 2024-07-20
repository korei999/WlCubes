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
struct HashMapIt
{
    T* pData;
    size_t hash;
    size_t idx;
    bool bInserted;
};

/* simple linear probing */
template<typename T>
struct HashMap
{
    BaseAllocator* allocator;
    Array<Bucket<T>> aBuckets;
    f64 maxLoadFactor;
    size_t bucketCount = 0;

    HashMap() = default;
    HashMap(BaseAllocator* pAllocator) : allocator(pAllocator), aBuckets(pAllocator), maxLoadFactor(HASHMAP_DEFAULT_LOAD_FACTOR) {}
    HashMap(BaseAllocator* pAllocator, size_t prealloc) : allocator(pAllocator), aBuckets(pAllocator, prealloc), maxLoadFactor(HASHMAP_DEFAULT_LOAD_FACTOR) {}

    Bucket<T>& operator[](size_t i) { return this->aBuckets[i]; }
    const Bucket<T>& operator[](size_t i) const { return this->aBuckets[i]; }

    f64 loadFactor() const { return static_cast<f64>(this->bucketCount) / static_cast<f64>(this->aBuckets.capacity); }
    size_t capacity() const { return this->aBuckets.capacity; }
    HashMapIt<T> insert(const T& value);
    HashMapIt<T> search(const T& value);
    void remove(size_t i);
    void rehash(size_t _size);
    HashMapIt<T> tryInsert(const T& value);
    void free() { this->aBuckets.free(); }
};

template<typename T>
inline HashMapIt<T>
HashMap<T>::insert(const T& value)
{
    if (this->loadFactor() >= this->maxLoadFactor)
        this->rehash(this->capacity() * 2);

    size_t hash = fnHash(value);
    size_t idx = hash % this->capacity();

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
inline HashMapIt<T>
HashMap<T>::search(const T& value)
{
    size_t hash = fnHash(value);
    size_t idx = hash % this->capacity();

    HashMapIt<T> ret;
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
HashMap<T>::remove(size_t i)
{
    this->aBuckets[i].bDeleted = true;
    this->aBuckets[i].bOccupied = false;
}

template<typename T>
inline void
HashMap<T>::rehash(size_t _size)
{
    auto mNew = HashMap<T>(this->aBuckets.allocator, _size);

    for (size_t i = 0; i < this->aBuckets.capacity; i++)
        if (this->aBuckets[i].bOccupied)
            mNew.insert(this->aBuckets[i].data);

    this->free();
    *this = mNew;
}

template<typename T>
inline HashMapIt<T>
HashMap<T>::tryInsert(const T& value)
{
    auto f = this->search(value);
    if (f.pData) return f;
    else return this->insert(value);
}

} /* namespace adt */
