#pragma once

#include "Allocator.hh"

namespace adt
{

struct DefaultAllocator : VIAllocator
{
    virtual void* alloc(size_t memberCount, size_t memberSize) override { return ::calloc(memberCount, memberSize); }
    virtual void free(void* p) override { ::free(p); }
    virtual void* realloc(void* p, size_t size) override { return ::realloc(p, size); }
};

inline DefaultAllocator StdAllocator {};

} /* namespace adt */
