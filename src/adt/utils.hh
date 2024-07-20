#pragma once

#include <time.h>
#include <stdio.h>

#include "ultratypes.h"

#define LEN(A) (sizeof(A) / sizeof(A[0]))
#define ODD(A) (A & 1)
#define EVEN(A) (!ODD(A))
#define NPOS static_cast<size_t>(-1UL)

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

template<typename A>
constexpr size_t
size(A& a)
{
    return LEN(a);
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
