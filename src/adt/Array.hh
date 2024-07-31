#pragma once

#include "Allocator.hh"

namespace adt
{

template<typename T>
struct Array
{
    Allocator* _pAlloc = nullptr;
    T* _pData = nullptr;
    u32 _size = 0;
    u32 _capacity = 0;

    Array() = default;
    Array(Allocator* pAlloc);
    Array(Allocator* pAlloc, u32 capacity);

    T& operator[](u32 i) { return _pData[i]; }
    const T& operator[](u32 i) const { return _pData[i]; }

    T* push(const T& data);
    T* pop();
    T& back() { return _pData[_size - 1]; }
    const T& back() const { return back(); }
    T& front() { return _pData[0]; }
    const T& front() const { front(); }
    T* data() { return _pData; }
    const T* data() const { return data(); }
    bool empty() const { return _size == 0;  }
    void resize(u32 _size);
    void grow(u32 _size);
    void destroy() { _pAlloc->free(_pData); }

    struct It
    {
        T* p_;

        It(T* _p) : p_(_p) {}

        T& operator*() const { return *p_; }
        T* operator->() const { return p_; }
        It operator++() { p_++; return *this; }
        It operator++(int) { It tmp = *this; (*this)++; return tmp; }
        friend bool operator==(const It& l, const It& r) { return l.p_ == r.p_; }
        friend bool operator!=(const It& l, const It& r) { return l.p_ != r.p_; }
    };

    It begin() { return &_pData[0]; }
    It end() { return &_pData[_size]; }
};

template<typename T>
Array<T>::Array(Allocator* _allocator)
    : Array<T>(_allocator, SIZE_MIN) {}

template<typename T>
Array<T>::Array(Allocator* pAlloc, u32 capacity)
    : _pAlloc(pAlloc), _pData((T*)(_pAlloc->alloc(capacity, sizeof(T)))), _size(0), _capacity(capacity) {}

template<typename T>
inline T*
Array<T>::push(const T& data)
{
    assert(_capacity > 0 && "pushing to the uninitialized array");

    if (_size >= _capacity)
        grow(_capacity * 2);

    _pData[_size++] = data;

    return &back();
}

template<typename T>
inline T*
Array<T>::pop()
{
    assert(!empty() && "popping from the empty array!");
    return &_pData[--_size];
}

template<typename T>
inline void
Array<T>::resize(u32 size)
{
    if (_size < size)
        grow(size);

    _size = size;
}

template<typename T>
inline void
Array<T>::grow(u32 size)
{
    _capacity = size;
    _pData = (T*)(_pAlloc->realloc(_pData, sizeof(T) * size));
}

} /* namespace adt */
