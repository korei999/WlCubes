#pragma once

#include "Allocator.hh"

namespace adt
{

struct DefaultAllocator : Allocator
{
    virtual void*
    alloc(size_t memberCount, size_t memberSize) override final
    {
        return ::calloc(memberCount, memberSize);
    }

    virtual void
    free(void* p) override final
    {
        ::free(p);
    }

    virtual void*
    realloc(void* p, size_t size) override final
    {
        return ::realloc(p, size);
    }
};

inline DefaultAllocator StdAllocator {};

} /* namespace adt */
