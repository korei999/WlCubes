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
    T* pData_;
    size_t hash_;
    u32 idx_;
    bool bInserted_;
};

/* `adt::fnHash<T>()` for hash function, linear probing */
template<typename T>
struct HashMap
{
    Allocator* pAlloc_;
    Array<Bucket<T>> aBuckets_;
    f64 maxLoadFactor_;
    u32 bucketCount_ = 0;

    HashMap() = default;
    HashMap(Allocator* pAllocator) : pAlloc_(pAllocator), aBuckets_(pAllocator, SIZE_MIN), maxLoadFactor_(HASHMAP_DEFAULT_LOAD_FACTOR) {}
    HashMap(Allocator* pAllocator, u32 prealloc) : pAlloc_(pAllocator), aBuckets_(pAllocator, prealloc), maxLoadFactor_(HASHMAP_DEFAULT_LOAD_FACTOR) {}

    Bucket<T>& operator[](u32 i) { return aBuckets_[i]; }
    const Bucket<T>& operator[](u32 i) const { return aBuckets_[i]; }

    f64 loadFactor() const { return static_cast<f64>(bucketCount_) / static_cast<f64>(aBuckets_.capacity_); }
    u32 capacity() const { return aBuckets_.capacity_; }
    HashMapRet<T> insert(const T& value);
    HashMapRet<T> search(const T& value);
    void remove(u32 i);
    void rehash(u32 _size);
    HashMapRet<T> tryInsert(const T& value);
    void destroy() { aBuckets_.destroy(); }
};

template<typename T>
inline HashMapRet<T>
HashMap<T>::insert(const T& value)
{
    if (loadFactor() >= maxLoadFactor_)
        rehash(capacity() * 2);

    size_t hash = fnHash(value);
    u32 idx = u32(hash % capacity());

    while (aBuckets_[idx].bOccupied)
    {
        idx++;
        if (idx >= capacity())
            idx = 0;
    }

    aBuckets_[idx].data = value;
    aBuckets_[idx].bOccupied = true;
    aBuckets_[idx].bDeleted = false;
    bucketCount_++;

    return {
        .pData = &aBuckets_[idx].data,
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
    u32 idx = u32(hash % capacity());

    HashMapRet<T> ret;
    ret.hash_ = hash;
    ret.pData_ = nullptr;
    ret.bInserted_ = false;

    while (aBuckets_[idx].bOccupied || aBuckets_[idx].bDeleted)
    {
        if (aBuckets_[idx].data == value)
        {
            ret.pData_ = &aBuckets_[idx].data;
            break;
        }

        idx++;
        if (idx >= capacity())
            idx = 0;
    }

    ret.idx_ = idx;
    return ret;
}

template<typename T>
inline void
HashMap<T>::remove(u32 i)
{
    aBuckets_[i].bDeleted = true;
    aBuckets_[i].bOccupied = false;
}

template<typename T>
inline void
HashMap<T>::rehash(u32 _size)
{
    auto mNew = HashMap<T>(aBuckets_.pAlloc, _size);

    for (u32 i = 0; i < aBuckets_.capacity_; i++)
        if (aBuckets_[i].bOccupied)
            mNew.insert(aBuckets_[i].data);

    destroy();
    *this = mNew;
}

template<typename T>
inline HashMapRet<T>
HashMap<T>::tryInsert(const T& value)
{
    auto f = search(value);
    if (f.pData) return f;
    else return insert(value);
}

} /* namespace adt */
