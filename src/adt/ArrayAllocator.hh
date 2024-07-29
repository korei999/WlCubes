#pragma once

#include <string.h>

#include "Array.hh"
#include "DefaultAllocator.hh"

namespace adt
{

struct ArrayAllocatorNode
{
    u64 selfIdx; /* 8 byte alignment for allocators */
    u8 pData[];
};

struct ArrayAllocator : Allocator
{
    Array<void*> aFreeList;

    ArrayAllocator() : aFreeList(&StdAllocator) {}
    ArrayAllocator(u32 prealloc) : aFreeList(&StdAllocator, prealloc) {}

    virtual void* alloc(u32 memberCount, u32 memberSize) override;
    virtual void free(void* p) override;
    virtual void* realloc(void* p, u32 size) override;
    void freeAll();

private:
    static ArrayAllocatorNode* ptrToNode(const void* p) { return (ArrayAllocatorNode*)((u8*)p - sizeof(ArrayAllocatorNode)); }
};

inline void*
ArrayAllocator::alloc(u32 memberCount, u32 memberSize)
{
    void* r = ::malloc(memberCount*memberSize + sizeof(ArrayAllocatorNode));
    memset(r, 0, memberCount*memberSize + sizeof(ArrayAllocatorNode));

    auto* pNode = (ArrayAllocatorNode*)r;
    this->aFreeList.push(pNode);
    pNode->selfIdx = this->aFreeList.size - 1; /* keep idx of this allocation in array to free later */

    return pNode->pData;
}

inline void
ArrayAllocator::free(void* p)
{
    auto* pNode = ptrToNode(p);
    assert(this->aFreeList[node->selfIdx] != nullptr && "double free");
    this->aFreeList[pNode->selfIdx] = nullptr;
    ::free(pNode);
}

inline void*
ArrayAllocator::realloc(void* p, u32 size)
{
    auto* pNode = ptrToNode(p);
    u64 idx = pNode->selfIdx;

    void* r = ::realloc(pNode, size + sizeof(ArrayAllocatorNode));
    auto pNew = (ArrayAllocatorNode*)r;

    this->aFreeList[idx] = nullptr;
    this->aFreeList.push(pNew);
    pNew->selfIdx = this->aFreeList.size - 1;

    return pNew->pData;
}

inline void
ArrayAllocator::freeAll()
{
    for (void* e : this->aFreeList)
        if (e)
        {
            ::free(e);
            e = nullptr;
        }

    this->aFreeList.destroy();
}

} /* namespace adt */
