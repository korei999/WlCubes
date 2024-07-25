#include "Arena.hh"
#include "frame.hh"
#include "Text.hh"

void
Text::genMesh(int xOrigin, int yOrigin, GLint drawMode, u32 size)
{
    adt::Arena allocScope(adt::SIZE_1M);

    auto aQuads = this->genBuffer(&allocScope, this->str, xOrigin, yOrigin, size);

    glGenVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, aQuads.size * sizeof(f32) * 4 * 6, aQuads.data(), drawMode);

    /* positions */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)0);
    /* texture coords */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)(2 * sizeof(f32)));

    this->vboSize = aQuads.size * 6; /* 6 vertices for 1 quad */

    glBindVertexArray(0);

    allocScope.freeAll();
}

adt::Array<TextCharQuad>
Text::genBuffer(adt::VIAllocator* pAlloc, adt::String s, int xOrigin, int yOrigin, u32 size)
{
    adt::Array<TextCharQuad> aQuads(pAlloc, size);
    memset(aQuads.pData, 0, sizeof(TextCharQuad) * size);

    /* 16/16 bitmap aka extended ascii */
    auto getUV = [](int p) -> f32 {
        return (1.0f / 16.0f) * p;
    };

    f32 xOff = 0.0f;
    f32 yOff = 0.0f;
    for (char c : s)
    {
        /* tl */
        f32 x0 = getUV(c % 16);
        f32 y0 = getUV(16 - (c / 16));

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
            -0.0f + xOff + xOrigin,  2.0f + yOff + frame::uiScale - 2.0f - yOrigin,     x0, y0, /* tl */
            -0.0f + xOff + xOrigin, -0.0f + yOff + frame::uiScale - 2.0f - yOrigin,     x1, y1, /* bl */
             2.0f + xOff + xOrigin, -0.0f + yOff + frame::uiScale - 2.0f - yOrigin,     x2, y2, /* br */

            -0.0f + xOff + xOrigin,  2.0f + yOff + frame::uiScale - 2.0f - yOrigin,     x0, y0, /* tl */
             2.0f + xOff + xOrigin, -0.0f + yOff + frame::uiScale - 2.0f - yOrigin,     x2, y2, /* br */
             2.0f + xOff + xOrigin,  2.0f + yOff + frame::uiScale - 2.0f - yOrigin,     x3, y3, /* tr */
        });

        xOff += 2.0f;
    }

    return aQuads;
}

void
Text::update(adt::VIAllocator* pAlloc, adt::String s, int x, int y)
{
    u32 oldSize = this->str.size;

    this->str = s;
    auto aQuads = this->genBuffer(pAlloc, s, x, y, oldSize);

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, adt::max(oldSize, s.size) * sizeof(f32) * 4 * 6, aQuads.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    aQuads.destroy();
}

void
Text::draw()
{
    glBindVertexArray(this->vao);
    glDrawArrays(GL_TRIANGLES, 0, this->vboSize);
}
