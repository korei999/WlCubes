#pragma once

namespace adt
{

template<typename A, typename B>
struct Pair
{
    union {
        A first;
        A a;
        A x;
    };
    union {
        A second;
        A b;
        A y;
    };
};

} /* namespace adt */
