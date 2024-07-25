#pragma once

#include "Allocator.hh"
#include "utils.hh"
#include "String.hh"

namespace parser
{

struct Binary
{
    adt::VIAllocator* pAlloc;
    adt::String word;
    adt::String file;
    u32 start;
    u32 end;

    Binary(adt::VIAllocator* p) : pAlloc(p) {}
    Binary(adt::VIAllocator* p, adt::String path) : Binary(p) { this->loadFile(path); }

    char& operator[](u32 i) { return file[i]; };

    void loadFile(adt::String path);
    void skipBytes(u32 n);
    adt::String readString(u32 size);
    u8 read8();
    u16 read16();
    u32 read32();
    u64 read64();
    void setPos(u32 p);
    u32 size() const { return file.size; };
    bool finished();
};

template <typename T>
#ifdef __linux__
__attribute__((no_sanitize("undefined"))) /* unaligned pointers */
#endif
T
readTypeBytes(adt::String vec, u32 i)
{
    return *reinterpret_cast<T*>(&vec[i]);
}

} /* namespace parser */
