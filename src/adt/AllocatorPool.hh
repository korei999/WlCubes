#pragma once

#include "ultratypes.h"

namespace adt
{

template<typename A, u32 MAX>
struct AllocatorPool
{
    A aAllocators[MAX];
    u32 size;
    u32 cap;

    AllocatorPool() : size(0), cap(MAX) {}

    A*
    get(u32 size)
    {
        assert(this->size < this->cap && "size reached cap");
        this->aAllocators[this->size++] = A(size);
        return &this->aAllocators[this->size - 1];
    }

    void
    freeAll()
    {
        for (auto& a : this->aAllocators)
            a.freeAll();
    }
};

} /* namespace adt */
