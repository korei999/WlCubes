#pragma once

#include <math.h>
#include <string.h>
#include <stddef.h>

#include "Allocator.hh"

#ifdef DEBUG
#include "logs.hh"
#endif

#define ARENA_FIRST(A) ((A)->_pBlocks)
#define ARENA_NEXT(AB) ((AB)->pNext)
#define ARENA_FOREACH(A, IT) for (ArenaBlock* IT = ARENA_FIRST(A); IT; IT = ARENA_NEXT(IT))
#define ARENA_FOREACH_SAFE(A, IT, TMP) for (ArenaBlock* IT = ARENA_FIRST(A), * TMP = nullptr; IT && ((TMP) = ARENA_NEXT(IT), true); (IT) = (TMP))

#define ARENA_NODE_GET_FROM_DATA(PDATA)   ((adt::ArenaNode*)((u8*)(PDATA)  - offsetof(adt::ArenaNode,  pData)))
#define ARENA_NODE_GET_FROM_BLOCK(PBLOCK) ((adt::ArenaNode*)((u8*)(PBLOCK) + offsetof(adt::ArenaBlock, pData)))

namespace adt
{

struct ArenaBlock
{
    ArenaBlock* pNext = nullptr;
    u8 pData[]; /* flexible array member */
};

struct ArenaNode
{
    ArenaNode* pNext = nullptr;
    ArenaBlock* pBlock;
    u64 size = 0;
    u8 pData[];
};

struct ArenaAllocator : Allocator
{
    ArenaBlock* _pBlocks = nullptr;
    size_t _blockSize = 0;
    ArenaNode* _pLatest = nullptr;
    ArenaBlock* _pLatestBlock = nullptr;

    ArenaAllocator() = default;
    ArenaAllocator(u32 cap);

    void reset();
    size_t alignedBytes(u32 bytes);
    virtual void* alloc(u32 memberCount, u32 size) override;
    virtual void free(void* p) override;
    virtual void* realloc(void* p, u32 size) override;
    void freeAll();

private:
    ArenaBlock* newBlock();
    ArenaBlock* getFreeBlock();
};

inline 
ArenaAllocator::ArenaAllocator(u32 cap)
    : _blockSize(alignedBytes(cap + sizeof(ArenaNode))) /* preventively align */
{
    newBlock();
}

inline void
ArenaAllocator::reset()
{
    ARENA_FOREACH(this, pB)
    {
        ArenaNode* pNode = ARENA_NODE_GET_FROM_BLOCK(pB);
        pNode->pNext = pNode;
    }

    auto first = ARENA_FIRST(this);
    ArenaNode* pNode = ARENA_NODE_GET_FROM_BLOCK(first);
    _pLatest = pNode;
    _pLatestBlock = first;
}

inline ArenaBlock*
ArenaAllocator::newBlock()
{
    ArenaBlock** ppLastBlock = &_pBlocks;
    while (*ppLastBlock) ppLastBlock = &((*ppLastBlock)->pNext);

    *ppLastBlock = (ArenaBlock*)(malloc(sizeof(ArenaBlock) + _blockSize));
    memset(*ppLastBlock, 0, sizeof(ArenaBlock) + _blockSize);

    auto* pNode = ARENA_NODE_GET_FROM_BLOCK(*ppLastBlock);
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

inline size_t
ArenaAllocator::alignedBytes(u32 bytes)
{
    f64 mulOf = f64(bytes) / f64(sizeof(size_t));
    return u32(sizeof(size_t) * ceil(mulOf));
}

inline void*
ArenaAllocator::alloc(u32 memberCount, u32 memberSize)
{
    ArenaBlock* pFreeBlock = _pLatestBlock;
    u32 requested = memberCount * memberSize;
    size_t aligned = alignedBytes(requested + sizeof(ArenaNode));

#ifdef DEBUG
    if (aligned >= _blockSize)
        LOG_FATAL("requested size is > than one block\n"
                  "aligned: %zu, blockSize: %zu, requested: %u\n", aligned, _blockSize, requested);
#endif

repeat:
    /* skip pNext */
    ArenaNode* pFreeBlockOff = ARENA_NODE_GET_FROM_BLOCK(pFreeBlock);
    ArenaNode* pNode = _pLatest->pNext;
    size_t nextAligned = ((u8*)pNode + aligned) - (u8*)pFreeBlockOff;

    /* heap overflow */
    if (nextAligned >= _blockSize)
    {
#ifdef DEBUG
        LOG_WARN("heap overflow\n");
#endif

        pFreeBlock = pFreeBlock->pNext;
        if (!pFreeBlock) pFreeBlock = newBlock();
        goto repeat;
    }

    pNode->pNext = (ArenaNode*)((u8*)pNode + aligned);
    pNode->size = requested;
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
ArenaAllocator::realloc(void* p, u32 size)
{
    ArenaNode* pNode = ARENA_NODE_GET_FROM_DATA(p);
    auto* pBlockOff = ARENA_NODE_GET_FROM_BLOCK(pNode->pBlock);
    auto aligned = alignedBytes(size);
    size_t nextAligned = ((u8*)pNode + aligned) - (u8*)pBlockOff;

    if (pNode == _pLatest && nextAligned < _blockSize)
    {
        pNode->size = size;
        pNode->pNext = (ArenaNode*)((u8*)pNode + aligned + sizeof(ArenaNode));

        return p;
    }
    else
    {
        void* pR = alloc(size, 1);
        memcpy(pR, p, pNode->size);

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
