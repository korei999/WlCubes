#pragma once

#include <math.h>
#include <string.h>
#include <stddef.h>

#include "allocator.hh"

#define ARENA_FIRST(A) ((A)->pBlocks)
#define ARENA_NEXT(AB) ((AB)->pNext)
#define ARENA_FOREACH(A, IT) for (ArenaBlock* IT = ARENA_FIRST(A); IT; IT = ARENA_NEXT(IT))
#define ARENA_FOREACH_SAFE(A, IT, TMP) for (ArenaBlock* IT = ARENA_FIRST(A), * TMP = nullptr; IT && ((TMP) = ARENA_NEXT(IT), true); (IT) = (TMP))

#define ARENA_NODE_GET_FROM_DATA(PDATA) (reinterpret_cast<adt::ArenaNode*>(reinterpret_cast<u8*>(PDATA) - offsetof(adt::ArenaNode, pData)))
#define ARENA_NODE_GET_FROM_BLOCK(PBLOCK) (reinterpret_cast<adt::ArenaNode*>(reinterpret_cast<u8*>(PBLOCK) + offsetof(adt::ArenaBlock, pData)))

namespace adt
{

struct ArenaBlock
{
    ArenaBlock* pNext = nullptr;
    u8 pData[];
};

struct ArenaNode
{
    ArenaNode* pNext = nullptr;
    ArenaBlock* pBlock;
    size_t size = 0;
    u8 pData[]; /* flexible array member */
};

struct Arena : BaseAllocator
{
    ArenaBlock* pBlocks = nullptr;
    size_t blockSize = 0;
    ArenaNode* pLatest = nullptr;
    ArenaBlock* pLatestBlock = nullptr;

    Arena(size_t cap);

    void reset();
    size_t alignedBytes(size_t bytes);
    virtual void* alloc(size_t memberCount, size_t size) override;
    virtual void free(void* p) override;
    virtual void* realloc(void* p, size_t size) override;
    void freeAll();

private:
    ArenaBlock* newBlock();
    bool fitsNode(ArenaNode* pNode, size_t size);
    ArenaBlock* getFreeBlock();
};

inline 
Arena::Arena(size_t cap)
    : blockSize(alignedBytes(cap + sizeof(ArenaNode))) /* preventively align */
{
    this->newBlock();
}

inline void
Arena::reset()
{
    ARENA_FOREACH(this, pB)
    {
        ArenaNode* pNode = ARENA_NODE_GET_FROM_BLOCK(pB), * pNext = nullptr;

        while (pNode->pNext)
        {
            pNext = pNode->pNext;
            pNode->size = 0;
            pNode->pNext = nullptr;
            pNode = pNext;
        }
    }
}

inline ArenaBlock*
Arena::newBlock()
{
    ArenaBlock** ppLastBlock = &this->pBlocks;
    while (*ppLastBlock) ppLastBlock = &((*ppLastBlock)->pNext);

    *ppLastBlock = reinterpret_cast<ArenaBlock*>(malloc(sizeof(ArenaBlock) + this->blockSize));
    memset(*ppLastBlock, 0, sizeof(ArenaBlock) + this->blockSize);

    auto* pNode = ARENA_NODE_GET_FROM_BLOCK(*ppLastBlock);
    pNode->pNext = pNode; /* so we don't bump the very first one on `alloc()` */
    this->pLatest = pNode;
    this->pLatestBlock = *ppLastBlock;

    return *ppLastBlock;
}

inline bool
Arena::fitsNode(ArenaNode* pNode, size_t size)
{
    /* TODO: get max contiguous space if we get bunch of freed nodes in the row, while not overflowing */
    return size_t((u8*)pNode->pNext - (u8*)pNode) > size;
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
Arena::alignedBytes(size_t bytes)
{
    f64 mulOf = static_cast<f64>(bytes) / static_cast<f64>(sizeof(size_t));
    return sizeof(size_t) * ceil(mulOf);
}

inline void*
Arena::alloc(size_t memberCount, size_t memberSize)
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
        pFreeBlock = this->newBlock();
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
Arena::realloc(void* p, size_t size)
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

} /* namespace adt */
