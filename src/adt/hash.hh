#pragma once

namespace adt
{

template<typename T>
inline size_t
fnHash(T& x)
{
    return (x);
}

template<>
inline size_t
fnHash<size_t>(size_t& x)
{
    return x;
}

template<>
inline size_t
fnHash<void* const>(void* const& x)
{
    return reinterpret_cast<size_t>(x);
}

constexpr size_t
hashFNV(const char* str, u32 size)
{
    size_t hash = 0xCBF29CE484222325;
    for (size_t i = 0; i < size; i++)
        hash = (hash ^ size_t(str[i])) * 0x100000001B3;
    return hash;
}

} /* namespace adt */
