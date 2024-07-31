#pragma once

#include "ultratypes.h"

#include <assert.h>

namespace adt
{

template<typename A, u32 MAX>
struct AllocatorPool
{
    A _aAllocators[MAX];
    u32 _size;
    u32 _cap;

    AllocatorPool() : _size(0), _cap(MAX) {}

    A*
    get(u32 size)
    {
        assert(_size < _cap && "size reached cap");
        _aAllocators[_size++] = A(size);
        return &_aAllocators[_size - 1];
    }

    void
    freeAll()
    {
        for (auto& a : _aAllocators)
            a.freeAll();
    }
};

} /* namespace adt */
