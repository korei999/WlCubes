#pragma once

#include "Allocator.hh"

namespace adt
{

struct DefaultAllocator : Allocator
{
    virtual void*
    alloc(u32 memberCount, u32 memberSize) override final
    {
        return ::calloc(memberCount, memberSize);
    }

    virtual void
    free(void* p) override final
    {
        ::free(p);
    }

    virtual void*
    realloc(void* p, u32 size) override final
    {
        return ::realloc(p, size);
    }
};

inline DefaultAllocator StdAllocator {};

} /* namespace adt */
