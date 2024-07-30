#pragma once

#include "ArenaAllocator.hh"
#include "Array.hh"
#include "DefaultAllocator.hh"

namespace adt
{

struct AllocatorPool
{
    AllocatorPool(u32 prealloc) : aAllocators(&StdAllocator, prealloc) {}

    ArenaAllocator*
    get(u32 size)
    {
        this->aAllocators.push({size});

        return &this->aAllocators[aAllocators.size - 1];
    }

    void
    freeAll()
    {
        for (auto& a : this->aAllocators)
            a.freeAll();

        aAllocators.destroy();
    }

private:
    Array<ArenaAllocator> aAllocators;
};

} /* namespace adt */
