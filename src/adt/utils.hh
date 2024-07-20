#pragma once

#include <time.h>
#include <stdio.h>

#include "ultratypes.h"

#define NPOS static_cast<u32>(-1UL)
#define NPOSL static_cast<u64>(-1UL)

namespace adt
{

template<typename A, typename B>
constexpr A&
max(A& l, B& r)
{
    return l > r ? l : r;
}

template<typename A, typename B>
constexpr A&
min(A& l, B& r)
{
    return l < r ? l : r;
}

template<typename T>
constexpr size_t
size(const T& a)
{
    return sizeof(a) / sizeof(a[0]);
}

template<typename T>
constexpr bool
odd(const T& a)
{
    return a & 1;
}

template<typename T>
constexpr bool
even(const T& a)
{
    return !odd(a);
}

inline f64
timeNowMS()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    time_t micros = ts.tv_sec * 1000000000;
    micros += ts.tv_nsec;
    return micros / 1000000.0;
}

inline f64
timeNowS()
{
    return timeNowMS() / 1000.0;
}

} /* namespace adt */
