#pragma once

#include <math.h>
#include <string.h>
#include <stddef.h>

#include "Allocator.hh"
#include "ultratypes.h"

#ifdef DEBUG
    #include "logs.hh"
#endif

#define ALIGN_TO_8_BYTES(x) (((x) + 8 - 1) & (~(8 - 1)))

#define ARENA_FIRST(A) ((A)->_pBlocksHead)
#define ARENA_NEXT(AB) ((AB)->pNext)
#define ARENA_FOREACH(A, IT) for (ArenaBlock* IT = ARENA_FIRST(A); IT; IT = ARENA_NEXT(IT))
#define ARENA_FOREACH_SAFE(A, IT, TMP) for (ArenaBlock* IT = ARENA_FIRST(A), * TMP = nullptr; IT && ((TMP) = ARENA_NEXT(IT), true); (IT) = (TMP))

namespace adt
{

struct ArenaNode;

struct ArenaBlock
{
    ArenaBlock* pNext = nullptr;
    size_t size = 0;
    ArenaNode* pLast = nullptr;
    u8 pData[]; /* flexible array member */
};

struct ArenaNode
{
    ArenaNode* pNext = nullptr;
    u8 pData[];
};

struct ArenaAllocator : Allocator
{
    ArenaBlock* _pBlocksHead = nullptr;
    ArenaBlock* _pLastBlockAllocation = nullptr;

    ArenaAllocator() = default;
    ArenaAllocator(u32 cap);

    void reset();
    virtual void* alloc(size_t memberCount, size_t size) override final;
    virtual void free(void* p) override final;
    virtual void* realloc(void* p, size_t size) override final;
    void freeAll();

private:
    ArenaBlock* newBlock(size_t size);
    ArenaBlock* getFreeBlock();

    static ArenaNode* getNodeFromData(void* p) { return (ArenaNode*)((u8*)(p) - offsetof(ArenaNode, pData)); }
    static ArenaNode* getNodeFromBlock(void* p) { return (ArenaNode*)((u8*)(p) + offsetof(ArenaBlock, pData)); }
};

inline 
ArenaAllocator::ArenaAllocator(u32 cap)
{
    newBlock(ALIGN_TO_8_BYTES(cap + sizeof(ArenaNode)));
}

inline void
ArenaAllocator::reset()
{
    ARENA_FOREACH(this, pB)
    {
        ArenaNode* pNode = getNodeFromBlock(pB);
        pB->pLast = pNode;
        pNode->pNext = pNode;
    }

    auto first = ARENA_FIRST(this);
    _pLastBlockAllocation = first;
}

inline ArenaBlock*
ArenaAllocator::newBlock(size_t size)
{
    ArenaBlock** ppLastBlock = &_pBlocksHead;
    while (*ppLastBlock) ppLastBlock = &((*ppLastBlock)->pNext);

    size_t addedSize = size + sizeof(ArenaBlock);

    *ppLastBlock = (ArenaBlock*)(calloc(1, addedSize));

    auto* pBlock = (ArenaBlock*)*ppLastBlock;
    pBlock->size = size;
    auto* pNode = getNodeFromBlock(*ppLastBlock);
    pNode->pNext = pNode; /* don't bump the very first node on `alloc()` */
    pBlock->pLast = pNode;
    _pLastBlockAllocation = *ppLastBlock;

    return *ppLastBlock;
}

inline ArenaBlock*
ArenaAllocator::getFreeBlock()
{
    ArenaBlock* pCurrBlock = _pBlocksHead, * pFreeBlock = _pBlocksHead;
    while (pCurrBlock)
    {
        pFreeBlock = pCurrBlock;
        pCurrBlock = pCurrBlock->pNext;
    }

    return pFreeBlock;
}

inline void*
ArenaAllocator::alloc(size_t memberCount, size_t memberSize)
{
    size_t requested = memberCount * memberSize;
    size_t aligned = ALIGN_TO_8_BYTES(requested + sizeof(ArenaNode));

    /* TODO: find block that can fit */
    ArenaBlock* pFreeBlock = _pBlocksHead;

    while (aligned >= pFreeBlock->size)
    {
#ifdef DEBUG
        LOG_WARN("requested size > than one block\n"
                "aligned: %zu, blockSize: %zu, requested: %u\n", aligned, pFreeBlock->size, requested);
#endif

        pFreeBlock = pFreeBlock->pNext;
        if (!pFreeBlock) pFreeBlock = newBlock(aligned * 2); /* NOTE: trying to double too big of an array situation */
    }

repeat:
    /* skip pNext */
    ArenaNode* pNode = getNodeFromBlock(pFreeBlock);
    ArenaNode* pNextNode = pFreeBlock->pLast->pNext;
    size_t nextAligned = ((u8*)pNextNode + aligned) - (u8*)pNode;

    /* heap overflow */
    if (nextAligned >= pFreeBlock->size)
    {
#ifdef DEBUG
        LOG_WARN("block overflow\n");
#endif

        pFreeBlock = pFreeBlock->pNext;
        if (!pFreeBlock) pFreeBlock = newBlock(nextAligned);
        goto repeat;
    }

    pNextNode->pNext = (ArenaNode*)((u8*)pNextNode + aligned);
    pFreeBlock->pLast = pNextNode;

    return &pNextNode->pData;
}

inline void
ArenaAllocator::free([[maybe_unused]] void* p)
{
    /* no individual frees */
}

inline void*
ArenaAllocator::realloc(void* p, size_t size)
{
    ArenaNode* pNode = getNodeFromData(p);
    ArenaBlock* pBlock = nullptr;

    /* figure out which block this node belongs to */
    ARENA_FOREACH(this, pB)
        if ((u8*)p > (u8*)pB && ((u8*)pB + pB->size) > (u8*)p)
            pBlock = pB;

    assert(pBlock != nullptr && "block not found, bad pointer");

    auto aligned = ALIGN_TO_8_BYTES(size);
    size_t nextAligned = ((u8*)pNode + aligned) - (u8*)getNodeFromBlock(pBlock);

    if (pNode == pBlock->pLast && nextAligned < pBlock->size)
    {
        /* NOTE: + sizeof(ArenaNode) is necessary */
        pNode->pNext = (ArenaNode*)((u8*)pNode + aligned + sizeof(ArenaNode));

        return p;
    }
    else
    {
        void* pR = alloc(1, size);
        memcpy(pR, p, ((u8*)pNode->pNext - (u8*)pNode));

        return pR;
    }
}

inline void
ArenaAllocator::freeAll()
{
    ARENA_FOREACH_SAFE(this, pB, tmp)
        ::free(pB);
}

} /* namespace adt */
