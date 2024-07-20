#pragma once

#include "allocator.hh"
#include "utils.hh"
#include "String.hh"

namespace parser
{

struct Binary
{
    adt::BaseAllocator* pAlloc;
    adt::String word;
    adt::String file;
    size_t start;
    size_t end;

    Binary(adt::BaseAllocator* p) : pAlloc(p) {}
    Binary(adt::BaseAllocator* p, adt::String path) : Binary(p) { this->loadFile(path); }

    char& operator[](size_t i) { return file[i]; };

    void loadFile(adt::String path);
    void skipBytes(size_t n);
    adt::String readString(size_t size);
    u8 read8();
    u16 read16();
    u32 read32();
    u64 read64();
    void setPos(size_t p);
    size_t size() const { return file.size; };
    bool finished();
};

template <typename T>
#ifdef __linux__
__attribute__((no_sanitize("undefined"))) /* unaligned pointers */
#endif
T
readTypeBytes(adt::String vec, size_t i)
{
    return *reinterpret_cast<T*>(&vec[i]);
}

} /* namespace parser */
