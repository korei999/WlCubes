#pragma once

#include "Allocator.hh"

namespace adt
{

template<typename T>
struct Array
{
    Allocator* allocator;
    T* pData = nullptr;
    u32 size = 0;
    u32 capacity = 0;

    Array() = default;
    Array(Allocator* _allocator);
    Array(Allocator* _allocator, u32 _capacity);

    T& operator[](u32 i) { return this->pData[i]; }
    const T& operator[](u32 i) const { return this->pData[i]; }

    T* push(const T& data);
    T& back();
    T& front();
    T* data() { return this->pData; }
    bool empty() const { return this->size == 0;  }
    void resize(u32 _size);
    void grow(u32 _size);
    void destroy() { this->allocator->free(this->pData); }

    struct It
    {
        T* p;

        It(T* _p) : p(_p) {}

        T& operator*() const { return *p; }
        T* operator->() const { return p; }
        It& operator++() { this->p++; return *this; }
        It& operator++(int) { It tmp = *this; (*this)++; return tmp; }
        friend bool operator==(const It& l, const It& r) { return l.p == r.p; }
        friend bool operator!=(const It& l, const It& r) { return l.p != r.p; }
    };

    It begin() { return &this->pData[0]; }
    It end() { return &this->pData[this->size]; }
};

template<typename T>
Array<T>::Array(Allocator* _allocator)
    : allocator(_allocator), capacity(SIZE_MIN)
{
    pData = static_cast<T*>(this->allocator->alloc(this->capacity, sizeof(T)));
}

template<typename T>
Array<T>::Array(Allocator* _allocator, u32 _capacity)
    : allocator(_allocator), capacity(_capacity)
{
    pData = static_cast<T*>(this->allocator->alloc(this->capacity, sizeof(T)));
}

template<typename T>
inline T*
Array<T>::push(const T& data)
{
    if (this->size >= this->capacity)
        this->grow(this->capacity * 2);

    this->pData[this->size++] = data;

    return &this->back();
}

template<typename T>
inline T&
Array<T>::back()
{
    return this->pData[this->size - 1];
}

template<typename T>
inline T&
Array<T>::front()
{
    return this->pData[0];
}

template<typename T>
inline void
Array<T>::resize(u32 _size)
{
    if (this->size < _size)
        this->grow(_size);

    this->size = _size;
}

template<typename T>
inline void
Array<T>::grow(u32 _size)
{
    this->capacity = _size;
    this->pData = static_cast<T*>(this->allocator->realloc(this->pData, sizeof(T) * _size));
}

} /* namespace adt */
