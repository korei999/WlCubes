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

        COUT("returned: %p\n", &this->aAllocators.back());
        return &this->aAllocators.back();
    }

    void
    freeAll()
    {
        for (auto& a : this->aAllocators)
            a.freeAll();

        aAllocators.destroy();
    }

private:
    Array<A> aAllocators;
};

} /* namespace adt */
