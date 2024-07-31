#pragma once

#include "ultratypes.h"

namespace adt
{

template<typename T>
inline u64
fnHash(T& x)
{
    return (x);
}

template<>
inline u64
fnHash<u64>(u64& x)
{
    return x;
}

template<>
inline u64
fnHash<void* const>(void* const& x)
{
    return reinterpret_cast<u64>(x);
}

constexpr u64
hashFNV(const char* str, u32 size)
{
    u64 hash = 0xCBF29CE484222325;
    for (u64 i = 0; i < size; i++)
        hash = (hash ^ u64(str[i])) * 0x100000001B3;
    return hash;
}

} /* namespace adt */
