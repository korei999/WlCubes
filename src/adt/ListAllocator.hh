#pragma once

#include <string.h>

#include "Array.hh"
#include "DefaultAllocator.hh"
#include "logs.hh"

namespace adt
{

struct ListAllocatorNode
{
    u64 selfIdx; /* 8 byte alignment for allocators */
    u8 pData[];
};

struct ListAllocator : Allocator
{
    Array<void*> aFreeList;

    ListAllocator() : aFreeList(&StdAllocator) {}
    ListAllocator(u32 prealloc) : aFreeList(&StdAllocator, prealloc) {}

    virtual void* alloc(u32 memberCount, u32 memberSize) override;
    virtual void free(void* p) override;
    virtual void* realloc(void* p, u32 size) override;
    void freeAll();

private:
    static ListAllocatorNode* ptrToNode(void* p) { return (ListAllocatorNode*)((u8*)p - sizeof(ListAllocatorNode)); }
};

inline void*
ListAllocator::alloc(u32 memberCount, u32 memberSize)
{
    void* r = ::malloc(memberCount*memberSize + sizeof(ListAllocatorNode));
    memset(r, 0, memberCount*memberSize + sizeof(ListAllocatorNode));

    auto* pNode = (ListAllocatorNode*)r;
    this->aFreeList.push(pNode);
    pNode->selfIdx = this->aFreeList.size - 1; /* keep idx of this allocation in array to free later */

    return pNode->pData;
}

inline void
ListAllocator::free(void* p)
{
    auto node = *ptrToNode(p);
    ::free(node.pData);
    assert(this->aFreeList[node.selfIdx] != nullptr && "double free");
    this->aFreeList[node.selfIdx] = nullptr;
    COUT("idx: '%zu' freed\n", node.selfIdx);
}

inline void*
ListAllocator::realloc(void* p, u32 size)
{
    auto* pNode = ptrToNode(p);
    this->aFreeList[pNode->selfIdx] = nullptr;

    void* r = ::realloc(pNode, size + sizeof(ListAllocatorNode));
    auto pNew = (ListAllocatorNode*)r;
    this->aFreeList.push(pNew);
    pNew->selfIdx = this->aFreeList.size - 1;

    return pNew->pData;
}

inline void
ListAllocator::freeAll()
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
