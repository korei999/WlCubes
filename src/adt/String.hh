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
    char* _pData = nullptr;
    u32 _size = 0;

    constexpr String() = default;
    constexpr String(char* sNullTerminated) : _pData(sNullTerminated), _size(nullTermStringSize(sNullTerminated)) {}
    constexpr String(const char* sNullTerminated) : _pData(const_cast<char*>(sNullTerminated)), _size(nullTermStringSize(sNullTerminated)) {}
    constexpr String(char* pStr, u32 len) : _pData(pStr), _size(len) {}

    constexpr char& operator[](u32 i) { return _pData[i]; }
    constexpr const char& operator[](u32 i) const { return _pData[i]; }

    constexpr char* data() { return _pData; }
    constexpr bool endsWith(String other);

    struct It
    {
        char* p_;
        u32 i_;
        u32 size_;

        It(String* _self, u32 _i) : p_(_self->_pData), i_(_i), size_(_self->_size) {}

        char& operator*() const { return p_[i_]; }
        char* operator->() const { return &p_[i_]; }

        It operator++()
        {
            if (i_ >= (size_ - 1) || size_ == 0)
            {
                i_ = NPOS;
                return *this;
            }

            i_++;
            return *this;
        }

        It operator++(int) { It tmp = *this; ++(*this); return tmp; }

        friend bool operator==(const It& l, const It& r) { return l.i_ == r.i_; }
        friend bool operator!=(const It& l, const It& r) { return l.i_ != r.i_; }
    };

    It begin() { return {this, 0}; }
    It end() { return {this, NPOS}; }
};

constexpr bool
String::endsWith(String r)
{
    auto& l = *this;

    if (l._size < r._size)
        return false;

    for (int i = r._size - 1, j = l._size - 1; i >= 0; i--, j--)
        if (r[i] != l[j])
            return false;

    return true;
}

constexpr bool
operator==(const String& sL, const String& sR)
{
    auto m = min(sL._size, sR._size);
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
    for (int i = sv._size - 1; i >= 0; i--)
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
    return makeString(p, s._pData, s._size);
}

template<>
constexpr size_t
fnHash<String>(String& str)
{
    return hashFNV(str._pData, str._size);
}

template<>
constexpr size_t
fnHash<const String>(const String& str)
{
    return hashFNV(str._pData, str._size);
}

constexpr size_t
hashFNV(String str)
{
    return hashFNV(str._pData, str._size);
}

constexpr String
concat(Allocator* p, String l, String r)
{
    u32 len = l._size + r._size;
    char* ret = (char*)p->alloc(len + 1, sizeof(char));

    u32 pos = 0;
    for (u32 i = 0; i < l._size; i++, pos++)
        ret[pos] = l[i];
    for (u32 i = 0; i < r._size; i++, pos++)
        ret[pos] = r[i];

    ret[len] = '\0';

    return {ret, len};
}

} /* namespace adt */
