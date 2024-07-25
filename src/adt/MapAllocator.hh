#pragma once

#include <threads.h>

#include "HashMap.hh"
#include "DefaultAllocator.hh"

namespace adt
{

struct MapAllocator : VIAllocator
{
    mtx_t mtxA;
    HashMap<void*> mPMap;

    MapAllocator() : mPMap(&StdAllocator, adt::SIZE_MIN) { mtx_init(&mtxA, mtx_plain); }
    MapAllocator(size_t prealloc) : mPMap(&StdAllocator, prealloc) {}

    virtual void* alloc(size_t memberCount, size_t memberSize) override;
    virtual void free(void* p) override;
    virtual void* realloc(void* p, size_t size) override;
    void freeAll();
    void destroy() { mtx_destroy(&this->mtxA); }
};

inline void*
MapAllocator::alloc(size_t memberCount, size_t memberSize)
{
    mtx_lock(&this->mtxA);
    void* pr = ::calloc(memberCount, memberSize);
    this->mPMap.insert(pr);
    mtx_unlock(&this->mtxA);

    return pr;
};

inline void
MapAllocator::free(void* p)
{
    mtx_lock(&this->mtxA);
    auto f = this->mPMap.search(p);
    if (f.pData) ::free(*f.pData);
    this->mPMap.remove(f.idx);
    mtx_unlock(&this->mtxA);
};

inline void*
MapAllocator::realloc(void* p, size_t size)
{
    auto f = this->mPMap.search(p);
    mtx_lock(&this->mtxA);
    if (f.pData) this->mPMap.remove(f.idx);

    auto pr = ::realloc(p, size);
    this->mPMap.insert(pr);
    mtx_unlock(&this->mtxA);

    return pr;
};

inline void
MapAllocator::freeAll()
{
    for (size_t i = 0; i < this->mPMap.capacity(); i++)
        if (this->mPMap[i].bOccupied)
        {
            ::free(this->mPMap[i].data);
            this->mPMap.remove(i);
        }

    this->mPMap.destroy();
}

} /* namespace adt */
