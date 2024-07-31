#pragma once

#include "Allocator.hh"
#include "utils.hh"

namespace adt
{

template<typename T>
struct Queue
{
    Allocator* _pAlloc {};
    T* _pData {};
    int _size {};
    int _capacity {};
    int _first {};
    int _last {};

    Queue() = default;
    Queue(Allocator* p) : Queue(p, SIZE_MIN) {}
    Queue(Allocator* p, u32 prealloc);

    T* pushBack(const T& val);
    T* popFront();
    T& front() { return _pData[firstI()]; }
    const T& front() const { return _pData[firstI()]; }
    T& back() { return _pData[lastI()]; }
    const T& back() const { return _pData[lastI()]; }
    T* data() { return &_pData[0]; }
    const T& data() const { return _pData[0]; }
    bool empty() const { return _size == 0; }
    void destroy() { _pAlloc->free(_pData); }
    void resize(u32 _size);
    int nextI(int i) { return (i + 1) >= _capacity ? 0 : (i + 1); }
    int prevI(int i) { return (i - 1) < 0 ? _capacity - 1 : (i - 1); }
    int firstI() const { return empty() ? -1 : _first; }
    int lastI() const { return empty() ? 0 : _last - 1; }

    T& operator[](int i) { return _pData[i]; }
    const T& operator[](int i) const { return _pData[i]; }

    struct It
    {
        Queue* _self {};
        int _i {};
        int _counter {};

        It(Queue* _self, int _i) : _self(_self), _i(_i) {}

        T& operator*() const { return _self->_pData[_i]; }
        T* operator->() const { return &_self->_pData[_i]; }

        It
        operator++()
        {
            if (_counter >= _self->_size - 1)
            {
                _i = NPOS;
                return *this;
            }

            _i = _self->nextI(_i);
            _counter++;

            return *this;
        }

        It operator++(int) { It tmp = *this; *this++; return tmp; }

        friend bool operator==(const It& l, const It& r) { return l._i == r._i; }
        friend bool operator!=(const It& l, const It& r) { return l._i != r._i; }
    };

    It begin() { return {this, firstI()}; }
    It end() { return {this, NPOS}; }
};

template<typename T>
inline
Queue<T>::Queue(Allocator* p, u32 prealloc)
    : _pAlloc(p), _capacity(prealloc)
{
    _pData = (T*)_pAlloc->alloc(prealloc, sizeof(T));
}

template<typename T>
inline T*
Queue<T>::pushBack(const T& val)
{
    if (_size >= _capacity)
        resize(_capacity * 2);
    
    int i = _last;
    int ni = nextI(i);
    _pData[i] = val;
    _last = ni;
    _size++;

    return &_pData[i];
}

template<typename T>
inline void
Queue<T>::resize(u32 size)
{
    auto nQ = Queue<T>(_pAlloc, size);

    for (int i = firstI(), t = 0; t < _size; i = nextI(i), t++)
        nQ.pushBack(_pData[i]);

    destroy();
    *this = nQ;
}


template<typename T>
inline T*
Queue<T>::popFront()
{
    assert(_size > 0);

    T* ret = &_pData[_first];
    _first = nextI(_first);
    _size--;

    return ret;
}

} /* namespace adt */
