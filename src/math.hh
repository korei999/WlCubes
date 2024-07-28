#pragma once

#include <math.h>

#include "ultratypes.h"

#ifdef LOGS
    #include "String.hh"
#endif

#define PI 3.14159265358979323846

template<typename T> T sq(T x) { return x * x; }
constexpr f64 toDeg(f64 x) { return x * 180.0 / PI; }
constexpr f64 toRad(f64 x) { return x * PI / 180.0; }
constexpr f32 toDeg(f32 x) { return x * 180.0f / static_cast<f32>(PI); }
constexpr f32 toRad(f32 x) { return x * static_cast<f32>(PI) / 180.0f; }

constexpr f64 toRad(long x) { return toRad(static_cast<f64>(x)); }
constexpr f64 toDeg(long x) { return toDeg(static_cast<f64>(x)); }
constexpr f32 toRad(int x) { return toRad(static_cast<f32>(x)); }
constexpr f32 toDeg(int x) { return toDeg(static_cast<f32>(x)); }

#define COLOR4(hex)                                                                                                    \
    {                                                                                                                  \
        ((hex >> 24) & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 16) & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 8)  & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 0)  & 0xFF) / 255.0f                                                                                  \
    }

#define COLOR3(hex)                                                                                                    \
    {                                                                                                                  \
        ((hex >> 16) & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 8)  & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 0)  & 0xFF) / 255.0f                                                                                  \
    }

union v2;
union v3;
union v4;
union qt;
union m3;

union v2
{
    struct {
        f32 x, y;
    };
    f32 e[2];

    v2() = default;
    v2(const v3& v);
    constexpr v2(f32 _x, f32 _y) : x(_x), y(_y) {}
};

union v3
{
    struct {
        f32 x, y, z;
    };
    struct {
        f32 r, g, b;
    };
    f32 e[3];

    v3() = default;
    v3(const v2& v);
    v3(const v4& v);
    constexpr v3(f32 _x, f32 _y, f32 _z) : x(_x), y(_y), z(_z) {}
    constexpr v3(u32 hex)
        : x(((hex >> 16) & 0xFF) / 255.0f),
          y(((hex >> 8)  & 0xFF) / 255.0f),
          z(((hex >> 0)  & 0xFF) / 255.0f) {}

    v3& operator+=(const v3& other);
    v3& operator-=(const v3& other);
    v3& operator*=(const f32 s);
};

union v4
{
    struct {
        f32 x, y, z, w;
    };
    struct {
        f32 r, g, b, a;
    };
    f32 e[4];

    v4() = default;
    constexpr v4(f32 _x, f32 _y, f32 _z, f32 _w) : x(_x), y(_y), z(_z), w(_w) {}
    v4(const qt& q);
};

union m4
{
    v4 v[4];
    f32 e[4][4];
    f32 p[16];

    m4() = default;
    constexpr m4(f32 _0, f32 _1, f32 _2, f32 _3, f32 _4, f32 _5, f32 _6, f32 _7, f32 _8, f32 _9, f32 _10, f32 _11, f32 _12, f32 _13, f32 _14, f32 _15)
        : p{_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15} {}
    m4(const m3& m);

    m4& operator*=(const m4& other);
};

union m3
{
    v3 v[4];
    f32 e[3][3];
    f32 p[9];

    m3() = default;
    m3(const m4& m) : v{m.v[0], m.v[1], m.v[2]} {}
    constexpr m3(f32 _0, f32 _1, f32 _2, f32 _3, f32 _4, f32 _5, f32 _6, f32 _7, f32 _8)
        : p{_0, _1, _2, _3, _4, _5, _6, _7, _8} {}
};

union qt
{
    struct {
        f32 x, y, z, s;
    };
    f32 p[4];

    qt() = default;
    constexpr qt(f32 _x, f32 _y, f32 _z, f32 _s) : x(_x), y(_y), z(_z), s(_s) {}
    constexpr qt(v4 _v) : x(_v.x), y(_v.y), z(_v.z), s(_v.w) {}
    constexpr qt(v3 _v, f32 _s) : x(_v.x), y(_v.y), z(_v.z), s(_s) {}
};

#ifdef LOGS
adt::String m4ToString(adt::Allocator* pAlloc, const m4& m, adt::String prefix);
adt::String m3ToString(adt::Allocator* pAlloc, const m3& m, adt::String prefix);
adt::String m4ToString(adt::Allocator* pAlloc, const m4& m, adt::String prefix);
#endif

constexpr m4
m4Iden()
{
    return m4 {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
}

constexpr m3
m3Iden()
{
    return {
        1, 0, 0,
        0, 1, 0,
        0, 0, 1
    };
}

constexpr qt
qtIden()
{
    return qt(0, 0, 0, 1);
}

f32 v3Length(const v3& v);
f32 v4Length(const v4& v);
v3 v3Norm(const v3& v); /* normilized (not length) */
v4 v4Norm(const v4& v); /* normilized (not length) */
v3 v3Norm(const v3& v, const f32 length); /* normilized (not length) */
v3 v3Cross(const v3& l, const v3& r);
v3 operator-(const v3& l, const v3& r);
v2 operator-(const v2& l, const v2& r);
v3 operator+(const v3& l, const v3& r);
v3 operator*(const v3& v, const f32 s);
f32 v3Rad(const v3& l, const v3& r); /* degree between two vectors */
f32 v3Dist(const v3& l, const v3& r); /* distance between two points in space */
f32 v3Dot(const v3& l, const v3& r);
f32 v4Dot(const v4& l, const v4& r);
m4 operator*(const m4& l, const m4& r);
m4 m4Rot(const m4& m, const f32 th, const v3& ax);
m4 m4RotX(const m4& m, const f32 angle);
m4 m4RotY(const m4& m, const f32 angle);
m4 m4RotZ(const m4& m, const f32 angle);
m4 m4Scale(const m4& m, const f32 s);
m4 m4Scale(const m4& m, const v3& s);
m4 m4Translate(const m4& m, const v3& tv);
m4 m4Pers(const f32 fov, const f32 asp, const f32 n, const f32 f);
m4 m4Ortho(const f32 l, const f32 r, const f32 b, const f32 t, const f32 n, const f32 f);
m4 m4LookAt(const v3& eyeV, const v3& centerV, const v3& upV);
m4 m4Transpose(const m4& m);
m3 m3Transpose(const m3& m);
m3 m3Inverse(const m3& m);
m3 m3Normal(const m3& m);
v3 v3Color(const u32 hex);
v4 v4Color(const u32 hex);
qt qtAxisAngle(const v3& axis, f32 th);
m4 qtRot(const qt& q);
qt qtConj(const qt& q);
qt operator*(const qt& l, const qt& r);
qt operator*=(qt& l, const qt& r);
