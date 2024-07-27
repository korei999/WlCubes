#pragma once

#include "HashMap.hh"
#include "DefaultAllocator.hh"

namespace adt
{

/* FIXME: `MapAllocator` causes deadlock (sometimes idk) */
struct MapAllocator : Allocator
{
    HashMap<void*> mPMap;

    MapAllocator() : mPMap(&StdAllocator, adt::SIZE_MIN) {}
    MapAllocator(u32 prealloc) : mPMap(&StdAllocator, prealloc) {}

    virtual void* alloc(u32 memberCount, u32 memberSize) override;
    virtual void free(void* p) override;
    virtual void* realloc(void* p, u32 size) override;
    void freeAll();
};

inline void*
MapAllocator::alloc(u32 memberCount, u32 memberSize)
{
    void* pr = ::calloc(memberCount, memberSize);
    this->mPMap.insert(pr);

    return pr;
};

inline void
MapAllocator::free(void* p)
{
    auto f = this->mPMap.search(p);
    ::free(*f.pData);
    this->mPMap.remove(f.idx);
};

inline void*
MapAllocator::realloc(void* p, u32 size)
{
    auto f = this->mPMap.search(p);
    this->mPMap.remove(f.idx);
    void* pr = ::realloc(p, size);
    this->mPMap.insert(pr);

    return pr;
};

inline void
MapAllocator::freeAll()
{
    for (u32 i = 0; i < this->mPMap.capacity(); i++)
        if (this->mPMap[i].bOccupied)
        {
            ::free(this->mPMap[i].data);
            this->mPMap.remove(i);
        }

    this->mPMap.destroy();
}

} /* namespace adt */
