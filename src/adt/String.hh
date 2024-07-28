#pragma once

#include "Allocator.hh"
#include "utils.hh"
#include "hash.hh"

namespace adt
{

constexpr u32
nullTermStringSize(const char* str)
{
    u32 i = 0;
    while (str[i] != '\0')
        i++;

    return i;
}

/* just pointer + size, no allocations, use `makeString()` for that */
struct String
{
    char* pData = nullptr;
    u32 size = 0;

    constexpr String() = default;
    constexpr String(char* sNullTerminated) : pData(sNullTerminated), size(nullTermStringSize(sNullTerminated)) {}
    constexpr String(const char* sNullTerminated) : pData(const_cast<char*>(sNullTerminated)), size(nullTermStringSize(sNullTerminated)) {}
    constexpr String(char* pStr, u32 len) : pData(pStr), size(len) {}

    constexpr char& operator[](u32 i) { return this->pData[i]; }
    constexpr const char& operator[](u32 i) const { return this->pData[i]; }

    constexpr char* data() { return this->pData; }
    constexpr bool endsWith(String other);

    struct It
    {
        char* p;
        u32 i;
        u32 size;

        It(String* _self, u32 _i) : p(_self->pData), i(_i), size(_self->size) {}

        char& operator*() const { return this->p[this->i]; }
        char* operator->() const { return &this->p[this->i]; }

        It operator++()
        {
            if (this->i >= (this->size - 1) || this->size == 0)
            {
                this->i = NPOS;
                return *this;
            }

            this->i++;
            return *this;
        }

        It operator++(int) { It tmp = *this; ++(*this); return tmp; }

        friend bool operator==(const It& l, const It& r) { return l.i == r.i; }
        friend bool operator!=(const It& l, const It& r) { return l.i != r.i; }
    };

    It begin() { return {this, 0}; }
    It end() { return {this, NPOS}; }
};

constexpr bool
String::endsWith(String r)
{
    auto& l = *this;

    if (l.size < r.size)
        return false;

    for (int i = r.size - 1, j = l.size - 1; i >= 0; i--, j--)
        if (r[i] != l[j])
            return false;

    return true;
}

constexpr bool
operator==(const String& sL, const String& sR)
{
    auto m = min(sL.size, sR.size);
    for (u32 i = 0; i < m; i++)
        if (sL[i] != sR[i])
            return false;

    return true;
}

constexpr bool
operator!=(const String& sL, const String& sR)
{
    return !(sL == sR);
}

constexpr u32
findLastOf(String sv, char c)
{
    for (int i = sv.size - 1; i >= 0; i--)
        if (sv[i] == c)
            return i;

    return NPOS;
}

constexpr String
makeString(Allocator* p, const char* str, u32 size)
{
    char* pData = (char*)(p->alloc(size + 1, sizeof(char)));
    for (u32 i = 0; i < size; i++)
        pData[i] = str[i];

    return {pData, size};
}

constexpr String
makeString(Allocator* p, u32 size)
{
    char* pData = (char*)(p->alloc(size + 1, sizeof(char)));
    return {pData, size};
}

constexpr String
makeString(Allocator* p, const char* str)
{
    return makeString(p, str, nullTermStringSize(str));
}

constexpr String
makeString(Allocator* p, String s)
{
    return makeString(p, s.pData, s.size);
}

template<>
constexpr size_t
fnHash<String>(String& str)
{
    return hashFNV(str.pData, str.size);
}

template<>
constexpr size_t
fnHash<const String>(const String& str)
{
    return hashFNV(str.pData, str.size);
}

constexpr size_t
hashFNV(String str)
{
    return hashFNV(str.pData, str.size);
}

constexpr String
concat(Allocator* p, String l, String r)
{
    u32 len = l.size + r.size;
    char* ret = (char*)p->alloc(len + 1, sizeof(char));

    u32 pos = 0;
    for (u32 i = 0; i < l.size; i++, pos++)
        ret[pos] = l[i];
    for (u32 i = 0; i < r.size; i++, pos++)
        ret[pos] = r[i];

    ret[len] = '\0';

    return {ret, len};
}

} /* namespace adt */
