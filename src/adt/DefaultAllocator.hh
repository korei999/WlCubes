#pragma once

#include "Allocator.hh"

namespace adt
{

struct DefaultAllocator : Allocator
{
    virtual void* alloc(u32 memberCount, u32 memberSize) override { return ::calloc(memberCount, memberSize); }
    virtual void free(void* p) override { ::free(p); }
    virtual void* realloc(void* p, u32 size) override { return ::realloc(p, size); }
};

inline DefaultAllocator StdAllocator {};

} /* namespace adt */
