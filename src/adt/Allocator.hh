#pragma once

#include <stdlib.h>
#include <assert.h>

#include "ultratypes.h"

namespace adt
{

using Nullptr = decltype(nullptr);

constexpr size_t SIZE_MIN = 8UL;
constexpr size_t SIZE_1K = 1024UL;
constexpr size_t SIZE_8K = 8 * SIZE_1K;
constexpr size_t SIZE_1M = SIZE_1K * SIZE_1K; 
constexpr size_t SIZE_8M = 8 * SIZE_1M; 
constexpr size_t SIZE_1G = SIZE_1M * SIZE_1K; 

/* Base allocator interface */
struct Allocator
{
    virtual void* alloc(u32 memberCount, u32 memberSize) = 0;
    virtual void free(void* p) = 0;
    virtual void* realloc(void* p, u32 size) = 0;
};

} /* namespace adt */
