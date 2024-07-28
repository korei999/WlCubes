#pragma once

#include <threads.h>

#include "HashMap.hh"
#include "DefaultAllocator.hh"

namespace adt
{

/* FIXME: `MapAllocator` causes deadlock (sometimes idk) */
struct MapAllocator : Allocator
{
    mtx_t mtxA;
    HashMap<void*> mPMap;

    MapAllocator() : mPMap(&StdAllocator, adt::SIZE_MIN) { mtx_init(&mtxA, mtx_plain); }
    MapAllocator(u32 prealloc) : mPMap(&StdAllocator, prealloc) {}

    virtual void* alloc(u32 memberCount, u32 memberSize) override;
    virtual void free(void* p) override;
    virtual void* realloc(void* p, u32 size) override;
    void freeAll();
    void destroy() { mtx_destroy(&this->mtxA); }
};

inline void*
MapAllocator::alloc(u32 memberCount, u32 memberSize)
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
MapAllocator::realloc(void* p, u32 size)
{
    mtx_lock(&this->mtxA);

    auto f = this->mPMap.search(p);
    if (f.pData) this->mPMap.remove(f.idx);

    auto pr = ::realloc(p, size);
    this->mPMap.insert(pr);

    mtx_unlock(&this->mtxA);

    return pr;
};

inline void
MapAllocator::freeAll()
{
    mtx_lock(&this->mtxA);

    for (u32 i = 0; i < this->mPMap.capacity(); i++)
        if (this->mPMap[i].bOccupied)
        {
            ::free(this->mPMap[i].data);
            this->mPMap.remove(i);
        }

    mtx_unlock(&this->mtxA);

    this->mPMap.destroy();
}

} /* namespace adt */
