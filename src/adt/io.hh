#pragma once

#include "String.hh"
#include "Array.hh"

namespace adt
{

inline String
loadFile(BaseAllocator* pAlloc, String path)
{
    String ret;

    auto sn = StringCreate(pAlloc, path);

    FILE* pf = fopen(sn.pData, "rb");
    if (pf)
    {
        fseek(pf, 0, SEEK_END);
        size_t size = ftell(pf) + 1;
        rewind(pf);

        ret.pData = static_cast<char*>(pAlloc->alloc(size, sizeof(char)));
        ret.size = size - 1;
        fread(ret.pData, 1, ret.size, pf);

        fclose(pf);
    }

    return ret;
}

inline Array<u8>
loadFileToCharArray(BaseAllocator* pAlloc, String path)
{
    Array<u8> ret(pAlloc);

    FILE* pf = fopen(path.data(), "rb");
    if (pf)
    {
        fseek(pf, 0, SEEK_END);
        size_t size = ftell(pf);
        rewind(pf);

        ret.resize(size + 1);
        ret.size = size;
        fread(ret.pData, 1, size, pf);

        fclose(pf);
    }

    return ret;
}

inline String
replacePathSuffix(BaseAllocator* pAlloc, adt::String path, adt::String suffix)
{
    auto lastSlash = adt::findLastOf(path, '/') + 1;
    adt::String pathToMtl = {&path[0], lastSlash};

    printf("path:      %.*s\n", (int)path.size, path.pData);
    printf("pathToMtl: %.*s\n", (int)pathToMtl.size, pathToMtl.pData);
    printf("suffix: %.*s\n", (int)suffix.size, suffix.pData);

    auto r = adt::catString(pAlloc, pathToMtl, suffix);

    printf("r: %s\n", r.pData);

    return r;
}

} /* namespace adt */
