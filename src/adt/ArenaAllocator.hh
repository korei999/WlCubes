#pragma once

#include <math.h>
#include <string.h>
#include <stddef.h>

#include "Allocator.hh"
#include "ultratypes.h"

#ifdef DEBUG
    #include "logs.hh"
#endif

#define ARENA_FIRST(A) ((A)->_pBlocks)
#define ARENA_NEXT(AB) ((AB)->pNext)
#define ARENA_FOREACH(A, IT) for (ArenaBlock* IT = ARENA_FIRST(A); IT; IT = ARENA_NEXT(IT))
#define ARENA_FOREACH_SAFE(A, IT, TMP) for (ArenaBlock* IT = ARENA_FIRST(A), * TMP = nullptr; IT && ((TMP) = ARENA_NEXT(IT), true); (IT) = (TMP))

#define ALIGN_TO8(x) (((x) + 8 - 1) & (~(8 - 1)))

namespace adt
{

struct ArenaBlock
{
    ArenaBlock* pNext = nullptr;
    size_t size = 0;
    u8 pData[]; /* flexible array member */
};

struct ArenaNode
{
    ArenaNode* pNext = nullptr;
    ArenaBlock* pBlock;
    u8 pData[];
};

struct ArenaAllocator : Allocator
{
    ArenaBlock* _pBlocks = nullptr;
    ArenaNode* _pLatest = nullptr;
    ArenaBlock* _pLatestBlock = nullptr;

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
    newBlock(ALIGN_TO8(cap + sizeof(ArenaNode)));
}

inline void
ArenaAllocator::reset()
{
    ARENA_FOREACH(this, pB)
    {
        ArenaNode* pNode = getNodeFromBlock(pB);
        pNode->pNext = pNode;
    }

    auto first = ARENA_FIRST(this);
    ArenaNode* pNode = getNodeFromBlock(first);
    _pLatest = pNode;
    _pLatestBlock = first;
}

inline ArenaBlock*
ArenaAllocator::newBlock(size_t size)
{
    ArenaBlock** ppLastBlock = &_pBlocks;
    while (*ppLastBlock) ppLastBlock = &((*ppLastBlock)->pNext);

    size_t addedSize = size + sizeof(ArenaBlock);

    *ppLastBlock = (ArenaBlock*)(calloc(1, addedSize));

    auto* pBlock = (ArenaBlock*)*ppLastBlock;
    pBlock->size = size;
    auto* pNode = getNodeFromBlock(*ppLastBlock);
    pNode->pNext = pNode; /* don't bump the very first node on `alloc()` */
    _pLatest = pNode;
    _pLatestBlock = *ppLastBlock;

    return *ppLastBlock;
}

inline ArenaBlock*
ArenaAllocator::getFreeBlock()
{
    ArenaBlock* pCurrBlock = _pBlocks, * pFreeBlock = _pBlocks;
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
    ArenaBlock* pFreeBlock = _pLatestBlock;
    u32 requested = memberCount * memberSize;
    size_t aligned = ALIGN_TO8(requested + sizeof(ArenaNode));

    if (aligned >= pFreeBlock->size)
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
    ArenaNode* pFreeBlockOff = getNodeFromBlock(pFreeBlock);
    ArenaNode* pNode = _pLatest->pNext;
    size_t nextAligned = ((u8*)pNode + aligned) - (u8*)pFreeBlockOff;

    /* heap overflow */
    if (nextAligned >= pFreeBlock->size)
    {
#ifdef DEBUG
        LOG_WARN("block overflow\n");
#endif

        size_t size = pFreeBlock->size;
        pFreeBlock = pFreeBlock->pNext;
        if (!pFreeBlock) pFreeBlock = newBlock(aligned);
        goto repeat;
    }

    pNode->pNext = (ArenaNode*)((u8*)pNode + aligned);
    pNode->pBlock = pFreeBlock;
    _pLatest = pNode;

    return &pNode->pData;
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
    auto* pBlockOff = getNodeFromBlock(pNode->pBlock);
    auto aligned = ALIGN_TO8(size);
    size_t nextAligned = ((u8*)pNode + aligned) - (u8*)pBlockOff;

    if (pNode == _pLatest && nextAligned < pNode->pBlock->size)
    {
        pNode->pNext = (ArenaNode*)((u8*)pNode + aligned + sizeof(ArenaNode));

        return p;
    }
    else
    {
        void* pR = alloc(size, 1);
        memcpy(pR, p, ((u8*)pNode->pNext - (u8*)pNode));

        return pR;
    }
}

inline void
ArenaAllocator::freeAll()
{
    ARENA_FOREACH_SAFE(this, it, tmp)
        ::free(it);
}

} /* namespace adt */
