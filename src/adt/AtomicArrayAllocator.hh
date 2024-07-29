#pragma once

#include <threads.h>

#include "ArrayAllocator.hh"

namespace adt
{

struct AtomicArrayAllocator : Allocator
{
    mtx_t mtx;
    ArrayAllocator lAlloc;

    AtomicArrayAllocator() { mtx_init(&this->mtx, mtx_plain); }
    AtomicArrayAllocator(u32 prealloc) : lAlloc(prealloc) { mtx_init(&this->mtx, mtx_plain); }

    virtual void* alloc(u32 memberCount, u32 memberSize) override;
    virtual void free(void* p) override;
    virtual void* realloc(void* p, u32 size) override;
    void freeAll();
};

inline void*
AtomicArrayAllocator::alloc(u32 memberCount, u32 memberSize)
{
    mtx_lock(&this->mtx);
    void* r = this->lAlloc.alloc(memberCount, memberSize);
    mtx_unlock(&this->mtx);

    return r;
}

inline void
AtomicArrayAllocator::free(void* p)
{
    mtx_lock(&this->mtx);
    this->lAlloc.free(p);
    mtx_unlock(&this->mtx);
}

inline void*
AtomicArrayAllocator::realloc(void* p, u32 size)
{
    mtx_lock(&this->mtx);
    void* r = this->lAlloc.realloc(p, size);
    mtx_unlock(&this->mtx);

    return r;
}

inline void
AtomicArrayAllocator::freeAll()
{
     this->lAlloc.freeAll();
     mtx_destroy(&this->mtx);
}

} /* namespace adt */
