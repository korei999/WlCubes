#include "Binary.hh"
#include "file.hh"

namespace parser
{

void 
Binary::loadFile(adt::String path)
{
    start = end = 0;
    file = adt::loadFile(this->pAlloc, path);
}

void 
Binary::skipBytes(u32 n)
{
    start = end = end + n;
}

adt::String
Binary::readString(u32 size)
{
    adt::String ret(&file[start], size - start);
    start = end = end + size;
    return ret;
}

u8
Binary::read8()
{
    auto ret = readTypeBytes<u8>(file, start);
    start = end = end + 1;
    return ret;
}

u16
Binary::read16()
{
    auto ret = readTypeBytes<u16>(file, start);
    start = end = end + 2;
    return ret;
}

u32
Binary::read32()
{
    auto ret = readTypeBytes<u32>(file, start);
    start = end = end + 4;
    return ret;
}

u64
Binary::read64()
{
    auto ret = readTypeBytes<u64>(file, start);
    start = end = end + 8;
    return ret;
}

void
Binary::setPos(u32 p)
{
    start = end = p;
}

bool
Binary::finished()
{
    return start >= size();
}

} /* namespace parser */
