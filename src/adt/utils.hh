#pragma once

#ifdef __linux__
    #include <time.h>
#elif _WIN32
    #include <windows.h>
    #include <sysinfoapi.h>
    #undef min
    #undef max
#endif

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
#ifdef __linux__
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    time_t micros = ts.tv_sec * 1000000000;
    micros += ts.tv_nsec;
    return micros / 1000000.0;
#elif _WIN32
    return static_cast<f64>(GetTickCount64());
#endif
}

inline f64
timeNowS()
{
    return timeNowMS() / 1000.0;
}

} /* namespace adt */
