#pragma once

#include "BaseAllocator.hh"

namespace adt
{

template<typename T>
struct Queue
{
    BaseAllocator* pAlloc {};
    T* pData {};
    int size {};
    int capacity {};
    int first {};
    int last {};

    Queue() = default;
    Queue(BaseAllocator* p) : Queue(p, SIZE_MIN) {}
    Queue(BaseAllocator* p, u32 prealloc);

    T* pushBack(const T& val);
    T* popFront();
    T& front() { return this->pData[this->firstI()]; }
    const T& front() const { return this->pData[this->firstI()]; }
    T& back() { return this->pData[this->lastI()]; }
    const T& back() const { return this->pData[this->lastI()]; }
    T* data() { return &this->pData[0]; }
    const T& data() const { return this->pData[0]; }
    bool empty() const { return this->size == 0; }
    void free() { this->pAlloc->free(this->pData); }
    void resize(u32 _size);
    int nextI(int i) { return (i + 1) >= this->capacity ? 0 : (i + 1); }
    int prevI(int i) { return (i - 1) < 0 ? this->capacity - 1 : (i - 1); }
    int firstI() const { return this->empty() ? -1 : this->first; }
    int lastI() const { return this->empty() ? 0 : this->last - 1; }

    T& operator[](int i) { return this->pData[i]; }
    const T& operator[](int i) const { return this->pData[i]; }

    struct It
    {
        Queue* self {};
        int i {};
        int counter {};

        It(Queue* _self, int _i) : self(_self), i(_i) {}

        T& operator*() const { return self->pData[this->i]; }
        T* operator->() const { return &self->pData[this->i]; }

        It&
        operator++()
        {
            if (this->counter >= self->size - 1)
            {
                this->i = -1;
                return *this;
            }

            this->i = this->self->nextI(i);
            this->counter++;

            return *this;
        }

        It& operator++(int) { It tmp = *this; *this++; return tmp; }

        friend bool operator==(const It& l, const It& r) { return l.i == r.i; }
        friend bool operator!=(const It& l, const It& r) { return l.i != r.i; }
    };

    It begin() { return {this, this->firstI()}; }
    It end() { return {this, -1}; }
};

template<typename T>
inline
Queue<T>::Queue(BaseAllocator* p, u32 prealloc)
    : pAlloc(p), capacity(prealloc)
{
    this->pData = (T*)this->pAlloc->alloc(prealloc, sizeof(T));
}

template<typename T>
inline T*
Queue<T>::pushBack(const T& val)
{
    if (this->size >= this->capacity)
        this->resize(this->capacity * 2);
    
    int i = this->last;
    int ni = this->nextI(i);
    this->pData[i] = val;
    this->last = ni;
    this->size++;

    return &this->pData[i];
}

template<typename T>
inline void
Queue<T>::resize(u32 _size)
{
    auto nQ = Queue<T>(this->pAlloc, _size);

    for (int i = this->firstI(), t = 0; t < this->size; i = this->nextI(i), t++)
        nQ.pushBack(this->pData[i]);

    this->free();
    *this = nQ;
}


template<typename T>
inline T*
Queue<T>::popFront()
{
    assert(this->size > 0);

    T* ret = &this->pData[this->first];
    this->first = this->nextI(this->first);
    this->size--;

    return ret;
}

} /* namespace adt */
