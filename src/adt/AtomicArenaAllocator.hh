#pragma once

#include <threads.h>

#include "ArenaAllocator.hh"

namespace adt
{

struct AtomicArenaAllocator : Allocator
{
    mtx_t _mtxA;
    ArenaAllocator _arena; /* compose for 1 mutex instead of second mutex for realloc (or recursive mutex) */

    AtomicArenaAllocator() = default;
    AtomicArenaAllocator(u32 blockSize) : _arena(blockSize) { mtx_init(&_mtxA, mtx_plain); }

    virtual void*
    alloc(u32 memberCount, u32 size) override final
    {
        mtx_lock(&_mtxA);
        auto rp = _arena.alloc(memberCount, size);
        mtx_unlock(&_mtxA);

        return rp;
    }

    virtual void
    free(void* p) override final
    {
        mtx_lock(&_mtxA);
        _arena.free(p);
        mtx_unlock(&_mtxA);
    }

    virtual void*
    realloc(void* p, u32 size) override final
    {
        mtx_lock(&_mtxA);
        auto rp = _arena.realloc(p, size);
        mtx_unlock(&_mtxA);

        return rp;
    }

    void
    reset()
    {
        mtx_lock(&_mtxA);
        _arena.reset();
        mtx_unlock(&_mtxA);
    }

    void
    freeAll()
    {
        _arena.freeAll();
        mtx_destroy(&_mtxA);
    }
};

} /* namespace adt */
