#pragma once

#include <threads.h>

#include "Arena.hh"

namespace adt
{

struct AtomicArena : Allocator
{
    mtx_t mtxA;
    Arena arena; /* compose for 1 mutex instead of second mutex for realloc (or recursive mutex) */

    AtomicArena() = default;
    AtomicArena(u32 blockSize) : arena(blockSize) { mtx_init(&this->mtxA, mtx_plain); }

    virtual void*
    alloc(u32 memberCount, u32 size) override
    {
        mtx_lock(&this->mtxA);
        auto rp = this->arena.alloc(memberCount, size);
        mtx_unlock(&this->mtxA);

        return rp;
    }

    virtual void
    free(void* p) override
    {
        mtx_lock(&this->mtxA);
        this->arena.free(p);
        mtx_unlock(&this->mtxA);
    }

    virtual void*
    realloc(void* p, u32 size) override
    {
        mtx_lock(&this->mtxA);
        auto rp = this->arena.realloc(p, size);
        mtx_unlock(&this->mtxA);

        return rp;
    }

    void reset() { this->arena.reset(); }
    void freeAll() { this->arena.freeAll(); }
    void destroy() { mtx_destroy(&this->mtxA); }
};

} /* namespace adt */
