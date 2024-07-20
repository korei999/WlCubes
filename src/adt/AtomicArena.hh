#pragma once

#include "Arena.hh"
#include <threads.h>

namespace adt
{

struct AtomicArena : Arena
{
    mtx_t mtxA;

    AtomicArena(size_t cap) : Arena(cap) { mtx_init(&mtxA, mtx_recursive); }

    void* alloc(size_t memberCount, size_t memberSize);
    void* realloc(void* p, size_t size);
    void free(void* p);
};

inline void*
AtomicArena::alloc(size_t memberCount, size_t memberSize)
{
    mtx_lock(&this->mtxA);
    void* ret = this->Arena::alloc(memberCount, memberSize);
    mtx_unlock(&this->mtxA);

    return ret;
}

inline void*
AtomicArena::realloc(void* p, size_t size)
{
    mtx_lock(&this->mtxA);
    void* ret = this->Arena::realloc(p, size);
    mtx_unlock(&this->mtxA);

    return ret;
}

inline void
AtomicArena::free(void* p)
{
    mtx_lock(&this->mtxA);
    this->Arena::free(p);
    mtx_unlock(&this->mtxA);
}

} /* namespace adt */
