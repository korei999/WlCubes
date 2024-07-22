#pragma once

#include <threads.h>

#include "Arena.hh"

namespace adt
{

struct AtomicArena : Arena
{
    mtx_t mtxA;

    AtomicArena() = default;
    AtomicArena(size_t blockSize) : Arena(blockSize) { mtx_init(&this->mtxA, mtx_plain); }

    virtual void*
    alloc(size_t memberCount, size_t size) override
    {
        mtx_lock(&this->mtxA);
        auto rp = this->Arena::alloc(memberCount, size);
        mtx_unlock(&this->mtxA);

        return rp;
    }

    virtual void
    free(void* p) override
    {
        mtx_lock(&this->mtxA);
        this->Arena::free(p);
        mtx_unlock(&this->mtxA);
    }

    virtual void*
    realloc(void* p, size_t size) override
    {
        /* NOTE: memcpy case will lock mutex, and bump case should be lock free (i think). */
        return this->Arena::realloc(p, size);
    }

    void destroy() { mtx_destroy(&this->mtxA); }
};

} /* namespace adt */
