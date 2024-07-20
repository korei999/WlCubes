#pragma once

#include "String.hh"
#include "Array.hh"
#include "Arena.hh"

namespace json
{

enum class TAG
{
    NULL_,
    STRING,
    LONG,
    DOUBLE,
    ARRAY,
    OBJECT,
    BOOL
};

static const char* TAGStrings[] {
    "NULL_", "STRING", "LONG", "DOUBLE", "ARRAY", "OBJECT", "BOOL"
};

inline const char*
getTAGString(enum TAG t)
{
    return TAGStrings[static_cast<int>(t)];
}

struct Object;

union Val
{
    adt::Nullptr n;
    adt::String sv;
    long l;
    double d;
    adt::Array<Object> a;
    adt::Array<Object> o;
    bool b;
};

struct TagVal
{
    enum TAG tag;
    union Val val;
};

struct Object
{
    adt::String svKey;
    TagVal tagVal;
};

} /* namespace json */
