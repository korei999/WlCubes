#pragma once

#include <math.h>
#include <string.h>
#include <stddef.h>

#include "Allocator.hh"

#define ARENA_FIRST(A) ((A)->pBlocks)
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
    size_t size = 0;
    u8 pData[];
};

struct Arena : Allocator
{
    ArenaBlock* pBlocks = nullptr;
    size_t blockSize = 0;
    ArenaNode* pLatest = nullptr;
    ArenaBlock* pLatestBlock = nullptr;

    Arena() = default;
    Arena(u32 cap);

    void reset();
    size_t alignedBytes(u32 bytes);
    virtual void* alloc(u32 memberCount, u32 size) override;
    virtual void free(void* p) override;
    virtual void* realloc(void* p, u32 size) override;
    void freeAll();
    void destroy();

private:
    ArenaBlock* newBlock();
    ArenaBlock* getFreeBlock();
};

inline 
Arena::Arena(u32 cap)
    : blockSize(alignedBytes(cap + sizeof(ArenaNode))) /* preventively align */
{
    this->newBlock();
}

inline void
Arena::reset()
{
    ARENA_FOREACH(this, pB)
    {
        ArenaNode* pNode = ARENA_NODE_GET_FROM_BLOCK(pB);
        pNode->pNext = pNode;
    }

    auto first = ARENA_FIRST(this);
    ArenaNode* pNode = ARENA_NODE_GET_FROM_BLOCK(first);
    this->pLatest = pNode;
    this->pLatestBlock = first;
}

inline ArenaBlock*
Arena::newBlock()
{
    ArenaBlock** ppLastBlock = &this->pBlocks;
    while (*ppLastBlock) ppLastBlock = &((*ppLastBlock)->pNext);

    *ppLastBlock = reinterpret_cast<ArenaBlock*>(malloc(sizeof(ArenaBlock) + this->blockSize));
    memset(*ppLastBlock, 0, sizeof(ArenaBlock) + this->blockSize);

    auto* pNode = ARENA_NODE_GET_FROM_BLOCK(*ppLastBlock);
    pNode->pNext = pNode; /* don't bump the very first node on `alloc()` */
    this->pLatest = pNode;
    this->pLatestBlock = *ppLastBlock;

    return *ppLastBlock;
}

inline ArenaBlock*
Arena::getFreeBlock()
{
    ArenaBlock* pCurrBlock = this->pBlocks, * pFreeBlock = this->pBlocks;
    while (pCurrBlock)
    {
        pFreeBlock = pCurrBlock;
        pCurrBlock = pCurrBlock->pNext;
    }

    return pFreeBlock;
}

inline size_t
Arena::alignedBytes(u32 bytes)
{
    f64 mulOf = f64(bytes) / f64(sizeof(size_t));
    return u32(sizeof(size_t) * ceil(mulOf));
}

inline void*
Arena::alloc(u32 memberCount, u32 memberSize)
{
    auto* pFreeBlock = this->pLatestBlock;

    auto requested = memberCount * memberSize;
    auto aligned = this->alignedBytes(requested + sizeof(ArenaNode));
    assert(aligned <= this->blockSize && "requested size is > than one block");

repeat:
    /* skip pNext */
    auto* pFreeBlockOff = ARENA_NODE_GET_FROM_BLOCK(pFreeBlock);

    ArenaNode* pNode = this->pLatest->pNext;

    size_t nextAligned = ((u8*)pNode + aligned) - (u8*)pFreeBlockOff;

    /* heap overflow */
    if (nextAligned >= this->blockSize)
    {
        pFreeBlock = pFreeBlock->pNext;
        if (!pFreeBlock) pFreeBlock = this->newBlock();
        goto repeat;
    }

    pNode->pNext = (ArenaNode*)((u8*)pNode + aligned);
    pNode->size = requested;
    pNode->pBlock = pFreeBlock;
    this->pLatest = pNode;

    return &pNode->pData;
}

inline void
Arena::free([[maybe_unused]] void* p)
{
    /* no individual frees */
}

inline void*
Arena::realloc(void* p, u32 size)
{
    ArenaNode* pNode = ARENA_NODE_GET_FROM_DATA(p);
    auto* pBlockOff = ARENA_NODE_GET_FROM_BLOCK(pNode->pBlock);
    auto aligned = alignedBytes(size);
    size_t nextAligned = ((u8*)pNode + aligned) - (u8*)pBlockOff;

    if (pNode == this->pLatest && nextAligned < this->blockSize)
    {
        pNode->size = size;
        pNode->pNext = (ArenaNode*)((u8*)pNode + aligned + sizeof(ArenaNode));

        return p;
    }
    else
    {
        void* pR = this->alloc(size, 1);
        memcpy(pR, p, pNode->size);

        return pR;
    }
}

inline void
Arena::freeAll()
{
    ARENA_FOREACH_SAFE(this, it, tmp)
        ::free(it);
}

inline void
Arena::destroy()
{
    //
}

} /* namespace adt */
