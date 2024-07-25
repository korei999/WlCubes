#include "Arena.hh"
#include "Array.hh"
#include "Pair.hh"
#include "Text.hh"

constexpr adt::Pair<u8, u8> asciiToUVMap[255] {
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {{10}, {}},
    {},
    {},
    {},
    {},
    {},

    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},

    {{0},  {16 - 2}},
    {{1},  {16 - 2}},
    {{2},  {16 - 2}},
    {{3},  {16 - 2}},
    {{4},  {16 - 2}},
    {{5},  {16 - 2}},
    {{6},  {16 - 2}},
    {{7},  {16 - 2}},
    {{8},  {16 - 2}},
    {{9},  {16 - 2}},
    {{10}, {16 - 2}},
    {{11}, {16 - 2}},
    {{12}, {16 - 2}},
    {{13}, {16 - 2}},
    {{14}, {16 - 2}},
    {{15}, {16 - 2}},

    {{0},  {16 - 3}},
    {{1},  {16 - 3}},
    {{2},  {16 - 3}},
    {{3},  {16 - 3}},
    {{4},  {16 - 3}},
    {{5},  {16 - 3}},
    {{6},  {16 - 3}},
    {{7},  {16 - 3}},
    {{8},  {16 - 3}},
    {{9},  {16 - 3}},
    {{10}, {16 - 3}},
    {{11}, {16 - 3}},
    {{12}, {16 - 3}},
    {{13}, {16 - 3}},
    {{14}, {16 - 3}},
    {{15}, {16 - 3}},

    {{0},  {16 - 4}},
    {{1},  {16 - 4}},
    {{2},  {16 - 4}},
    {{3},  {16 - 4}},
    {{4},  {16 - 4}},
    {{5},  {16 - 4}},
    {{6},  {16 - 4}},
    {{7},  {16 - 4}},
    {{8},  {16 - 4}},
    {{9},  {16 - 4}},
    {{10}, {16 - 4}},
    {{11}, {16 - 4}},
    {{12}, {16 - 4}},
    {{13}, {16 - 4}},
    {{14}, {16 - 4}},
    {{15}, {16 - 4}},

    {{0},  {16 - 5}},
    {{1},  {16 - 5}},
    {{2},  {16 - 5}},
    {{3},  {16 - 5}},
    {{4},  {16 - 5}},
    {{5},  {16 - 5}},
    {{6},  {16 - 5}},
    {{7},  {16 - 5}},
    {{8},  {16 - 5}},
    {{9},  {16 - 5}},
    {{10}, {16 - 5}},
    {{11}, {16 - 5}},
    {{12}, {16 - 5}},
    {{13}, {16 - 5}},
    {{14}, {16 - 5}},
    {{15}, {16 - 5}},

    {{0},  {16 - 6}},
    {{1},  {16 - 6}},
    {{2},  {16 - 6}},
    {{3},  {16 - 6}},
    {{4},  {16 - 6}},
    {{5},  {16 - 6}},
    {{6},  {16 - 6}},
    {{7},  {16 - 6}},
    {{8},  {16 - 6}},
    {{9},  {16 - 6}},
    {{10}, {16 - 6}},
    {{11}, {16 - 6}},
    {{12}, {16 - 6}},
    {{13}, {16 - 6}},
    {{14}, {16 - 6}},
    {{15}, {16 - 6}},

    {{0},  {16 - 7}},
    {{1},  {16 - 7}},
    {{2},  {16 - 7}},
    {{3},  {16 - 7}},
    {{4},  {16 - 7}},
    {{5},  {16 - 7}},
    {{6},  {16 - 7}},
    {{7},  {16 - 7}},
    {{8},  {16 - 7}},
    {{9},  {16 - 7}},
    {{10}, {16 - 7}},
    {{11}, {16 - 7}},
    {{12}, {16 - 7}},
    {{13}, {16 - 7}},
    {{14}, {16 - 7}},
    {{15}, {16 - 7}},
};

void
Text::genMesh()
{
    adt::Arena allocScope(adt::SIZE_1M);

    /* 16/16 bitmap aka extended ascii */
    auto getUV = [](int p) -> f32 {
        return (1.0f / 16.0f) * p;
    };

    struct CharQuad
    {
        f32 vs[30];
    };

    adt::Array<CharQuad> aQuads(&allocScope);

    f32 xOff = 0.0f;
    f32 yOff = 0.0f;
    for (char c : this->str)
    {
        auto pair = asciiToUVMap[(int)c];

        /* tl */
        f32 x0 = getUV(pair.x);
        f32 y0 = getUV(pair.y);

        /* bl */
        f32 x1 = x0;
        f32 y1 = y0 - getUV(1);

        /* br */
        f32 x2 = x0 + getUV(1);
        f32 y2 = y1;

        /* tr */
        f32 x3 = x1 + getUV(1);
        f32 y3 = y0;

        if (c == '\n')
        {
            xOff = -0.0f;
            yOff -= 2.0f;
            continue;
        }

        aQuads.push({
                                       /* FIXME: magick UI scale? */
            -0.0f + xOff,  2.0f + yOff + 98.0f,  0.0f,    x0,  y0, /* tl */
            -0.0f + xOff, -0.0f + yOff + 98.0f,  0.0f,    x1,  y1, /* bl */
             2.0f + xOff, -0.0f + yOff + 98.0f,  0.0f,    x2,  y2, /* br */

            -0.0f + xOff,  2.0f + yOff + 98.0f,  0.0f,    x0,  y0, /* tl */
             2.0f + xOff, -0.0f + yOff + 98.0f,  0.0f,    x2,  y2, /* br */
             2.0f + xOff,  2.0f + yOff + 98.0f,  0.0f,    x3,  y3, /* tr */
        });

        xOff += 2.0f;
    }

    glGenVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, aQuads.size * sizeof(f32) * 8 * 6, aQuads.data(), GL_STATIC_DRAW);

    /* positions */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(f32), (void*)0);
    /* texture coords */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(f32), (void*)(3 * sizeof(f32)));

    this->vboSize = aQuads.size * 6; /* 6 vertices for 1 quad */

    glBindVertexArray(0);

    allocScope.freeAll();
}

void
Text::draw()
{
    glBindVertexArray(this->vao);
    glDrawArrays(GL_TRIANGLES, 0, this->vboSize);
}
