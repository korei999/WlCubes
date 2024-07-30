#pragma once

#include "ultratypes.h"

#ifdef DEBUG
    #include "logs.hh"
#endif

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
#ifdef DEBUG
        if (this->size >= this->cap)
            LOG_FATAL("Size reached cap\n");
#endif

        new(&this->aAllocators[this->size++]) A(size); /* 'placement new' */
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
