#pragma once

#include "BaseAllocator.hh"
#include "utils.hh"
#include "hash.hh"

namespace adt
{

constexpr size_t
nullTermStringSize(const char* str)
{
    size_t i = 0;
    while (str[i] != '\0')
        i++;

    return i;
}

struct String
{
    char* pData = nullptr;
    size_t size = 0;

    constexpr String() = default;
    constexpr String(char* sNullTerminated) : pData(sNullTerminated), size(nullTermStringSize(sNullTerminated)) {}
    constexpr String(const char* sNullTerminated) : pData(const_cast<char*>(sNullTerminated)), size(nullTermStringSize(sNullTerminated)) {}
    constexpr String(char* pStr, size_t len) : pData(pStr), size(len) {}

    constexpr char& operator[](size_t i) { return this->pData[i]; }
    constexpr const char& operator[](size_t i) const { return this->pData[i]; }

    constexpr char* data() { return this->pData; }
};

constexpr bool
operator==(const String& sL, const String& sR)
{
    auto m = min(sL.size, sR.size);
    for (size_t i = 0; i < m; i++)
        if (sL[i] != sR[i])
            return false;

    return true;
}

constexpr bool
operator!=(const String& sL, const String& sR)
{
    return !(sL == sR);
}

constexpr size_t
findLastOf(String sv, char c)
{
    for (long i = sv.size - 1; i >= 0; i--)
        if (sv[i] == c)
            return i;

    return NPOS;
}

constexpr String
StringCreate(BaseAllocator* p, const char* str, size_t size)
{
    char* pData = static_cast<char*>(p->alloc(size + 1, sizeof(char)));
    for (size_t i = 0; i < size; i++)
        pData[i] = str[i];

    return {pData, size};
}

constexpr String
StringCreate(BaseAllocator* p, const char* str)
{
    return StringCreate(p, str, nullTermStringSize(str));
}

constexpr String
StringCreate(BaseAllocator* p, String s)
{
    return StringCreate(p, s.pData, s.size);
}

template<>
constexpr size_t
fnHash<String>(String& str)
{
    return hashFNV(str.pData, str.size);
}

constexpr size_t
hashFNV(String str)
{
    return hashFNV(str.pData, str.size);
}

constexpr String
catString(BaseAllocator* p, String l, String r)
{
    size_t len = l.size + r.size;
    char* ret = (char*)p->alloc(len + 1, sizeof(char));

    size_t pos = 0;
    for (size_t i = 0; i < l.size; i++, pos++)
        ret[pos] = l[i];
    for (size_t i = 0; i < r.size; i++, pos++)
        ret[pos] = r[i];

    ret[len] = '\0';

    return {ret, len};
}

constexpr bool
endsWith(String l, String r)
{
    if (l.size < r.size)
        return false;

    for (size_t i = l.size - r.size; i < l.size; i++)
        if (l[i] != r[i])
            return false;

    return true;
}

} /* namespace adt */
