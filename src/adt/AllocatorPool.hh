#pragma once

#include "Array.hh"
#include "DefaultAllocator.hh"

namespace adt
{

template<typename A>
struct AllocatorPool
{
    AllocatorPool(u32 prealloc) : aAllocators(&StdAllocator, prealloc) {}

    A*
    get(u32 size)
    {
        this->aAllocators.push({});
        new (&this->aAllocators.back()) A(size); /* 'placement new' */

        return &this->aAllocators.back();
    }

    void
    freeAll()
    {
        for (auto& a : this->aAllocators)
            a.freeAll();

        this->aAllocators.destroy();
    }

private:
    Array<A> aAllocators;
};

} /* namespace adt */
