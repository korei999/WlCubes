#pragma once

#include <threads.h>

#include "ListAllocator.hh"

namespace adt
{

struct AtomicListAllocator : Allocator
{
    mtx_t mtx;
    ListAllocator lAlloc;

    AtomicListAllocator() { mtx_init(&this->mtx, mtx_plain); }
    AtomicListAllocator(u32 prealloc) : lAlloc(prealloc) { mtx_init(&this->mtx, mtx_plain); }

    virtual void* alloc(u32 memberCount, u32 memberSize) override;
    virtual void free(void* p) override;
    virtual void* realloc(void* p, u32 size) override;
    void freeAll() { this->lAlloc.freeAll(); }
    void destroy() { this->lAlloc.destroy(); mtx_destroy(&this->mtx); }
};

inline void*
AtomicListAllocator::alloc(u32 memberCount, u32 memberSize)
{
    mtx_lock(&this->mtx);
    void* r = this->lAlloc.alloc(memberCount, memberSize);
    mtx_unlock(&this->mtx);

    return r;
}

inline void
AtomicListAllocator::free(void* p)
{
    mtx_lock(&this->mtx);
    this->lAlloc.free(p);
    mtx_unlock(&this->mtx);
}

inline void*
AtomicListAllocator::realloc(void* p, u32 size)
{
    mtx_lock(&this->mtx);
    void* r = this->lAlloc.realloc(p, size);
    mtx_unlock(&this->mtx);

    return r;
}

} /* namespace adt */
