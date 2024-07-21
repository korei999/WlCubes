#include "Model.hh"
#include "logs.hh"
#include "file.hh"
#include "ThreadPool.hh"
#include "MapAllocator.hh"

void
Model::load(adt::String path, GLint drawMode, GLint texMode, App* c)
{
    /*if (adt::endsWith(path, ".gltf"))*/
        this->loadGLTF(path, drawMode, texMode, c);
    /*else*/
    /*    LOG_FATAL("trying to load unsupported asset: '%.*s'\n", (int)path.size, path.pData);*/

    this->savedPath = path;
}

void
Model::loadGLTF(adt::String path, GLint drawMode, GLint texMode, App* c)
{
    this->asset.load(path);
    auto& a = this->asset;;

    /* load buffers first */
    adt::Array<GLuint> aBufferMap(this->pAlloc);
    for (u32 i = 0; i < a.aBuffers.size; i++)
    {
        mtx_lock(&gl::mtxGlContext);
        c->bindGlContext();

        GLuint b;
        glGenBuffers(1, &b);
        glBindBuffer(GL_ARRAY_BUFFER, b);
        glBufferData(GL_ARRAY_BUFFER, a.aBuffers[i].byteLength, a.aBuffers[i].aBin.data(), drawMode);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        aBufferMap.push(b);

        c->unbindGlContext();
        mtx_unlock(&gl::mtxGlContext);
    }

    adt::MapAllocator aAlloc;

    adt::ThreadPool tp(&aAlloc);
    tp.start();

    /* preload texures */
    adt::Array<Texture> aTex(this->pAlloc, a.aImages.size);

    for (u32 i = 0; i < a.aImages.size; i++)
    {
        auto* p = &aTex[i];
        auto uri = a.aImages[i].uri;

        struct args {
            Texture* p;
            adt::BaseAllocator* pAlloc;
            adt::String path;
            TEX_TYPE type;
            bool flip;
            GLint texMode;
            App* c;
        };

        auto* arg = (args*)aAlloc.alloc(1, sizeof(args));
        *arg = {
            .p = p,
            .pAlloc = &aAlloc,
            .path = adt::replacePathSuffix(this->pAlloc, path, uri),
            .type = TEX_TYPE::DIFFUSE,
            .flip = true,
            .texMode = texMode,
            .c = c
        };

        auto task = [](void* pArgs) -> int {
            auto a = *(args*)pArgs;
            *a.p = Texture(a.pAlloc, a.path, a.type, a.flip, a.texMode, a.c);

            return 0;
        };

        tp.submit(task, arg);
    }

    tp.wait();

    u32 meshIdx = NPOS;
    for (auto& mesh : a.aMeshes)
    {
        adt::Array<Mesh> aNMeshes(this->pAlloc);

        for (auto& primitive : mesh.aPrimitives)
        {
            u32 accIndIdx = primitive.indices;
            u32 accPosIdx = primitive.attributes.POSITION;
            u32 accNormIdx = primitive.attributes.NORMAL;
            u32 accTexIdx = primitive.attributes.TEXCOORD_0;
            u32 accTanIdx = primitive.attributes.TANGENT;
            u32 accMatIdx = primitive.material;
            enum gltf::PRIMITIVES mode = primitive.mode;

            auto& accPos = a.aAccessors[accPosIdx];
            auto& accTex = a.aAccessors[accTexIdx];

            auto& bvPos = a.aBufferViews[accPos.bufferView];
            auto& bvTex = a.aBufferViews[accTex.bufferView];

            Mesh nMesh {};

            nMesh.mode = mode;

            mtx_lock(&gl::mtxGlContext);
            c->bindGlContext();

            glGenVertexArrays(1, &nMesh.meshData.vao);
            glBindVertexArray(nMesh.meshData.vao);

            if (accIndIdx != NPOS)
            {
                auto& accInd = a.aAccessors[accIndIdx];
                auto& bvInd = a.aBufferViews[accInd.bufferView];
                nMesh.indType = accInd.componentType;
                nMesh.meshData.eboSize = accInd.count;
                nMesh.triangleCount = NPOS;

                /* TODO: figure out how to reuse VBO data for index buffer (possible?) */
                glGenBuffers(1, &nMesh.meshData.ebo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nMesh.meshData.ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, bvInd.byteLength,
                             &a.aBuffers[bvInd.buffer].aBin.data()[bvInd.byteOffset + accInd.byteOffset], drawMode);
            }
            else
            {
                nMesh.triangleCount = accPos.count;
            }

            constexpr u32 v3Size = sizeof(v3) / sizeof(f32);
            constexpr u32 v2Size = sizeof(v2) / sizeof(f32);

            /* if there are different VBO's for positions textures or normals,
             * given gltf file should be considered harmful, and this will crash ofc */
            nMesh.meshData.vbo = aBufferMap[bvPos.buffer];
            glBindBuffer(GL_ARRAY_BUFFER, nMesh.meshData.vbo);

            /* positions */
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, v3Size, static_cast<GLenum>(accPos.componentType), GL_FALSE,
                                  bvPos.byteStride, reinterpret_cast<void*>(bvPos.byteOffset + accPos.byteOffset));

            /* texture coords */
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, v2Size, static_cast<GLenum>(accTex.componentType), GL_FALSE,
                                  bvTex.byteStride, reinterpret_cast<void*>(bvTex.byteOffset + accTex.byteOffset));

             /*normals */
            if (accNormIdx != NPOS)
            {
                auto& accNorm = a.aAccessors[accNormIdx];
                auto& bvNorm = a.aBufferViews[accNorm.bufferView];

                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, v3Size, static_cast<GLenum>(accNorm.componentType), GL_FALSE,
                                      bvNorm.byteStride, reinterpret_cast<void*>(accNorm.byteOffset + bvNorm.byteOffset));
            }

            /* tangents */
            if (accTanIdx != NPOS)
            {
                auto& accTan = a.aAccessors[accTanIdx];
                auto& bvTan = a.aBufferViews[accTan.bufferView];

                glEnableVertexAttribArray(3);
                glVertexAttribPointer(3, v3Size, static_cast<GLenum>(accTan.componentType), GL_FALSE,
                                      bvTan.byteStride, reinterpret_cast<void*>(accTan.byteOffset + bvTan.byteOffset));
            }

            glBindVertexArray(0);
            c->unbindGlContext();
            mtx_unlock(&gl::mtxGlContext);

            /* load textures */
            if (accMatIdx != NPOS)
            {
                auto& mat = a.aMaterials[accMatIdx];
                u32 baseColorSourceIdx = mat.pbrMetallicRoughness.baseColorTexture.index;

                if (baseColorSourceIdx != NPOS)
                {
                    u32 diffTexInd = a.aTextures[baseColorSourceIdx].source;
                    if (diffTexInd != NPOS)
                    {
                        nMesh.meshData.materials.diffuse = aTex[diffTexInd];
                        nMesh.meshData.materials.diffuse.type = TEX_TYPE::DIFFUSE;
                    }
                }

                u32 normalSourceIdx = mat.normalTexture.index;
                if (normalSourceIdx != NPOS)
                {
                    u32 normTexIdx = a.aTextures[normalSourceIdx].source;
                    if (normTexIdx != NPOS)
                    {
                        nMesh.meshData.materials.normal = aTex[normalSourceIdx];
                        nMesh.meshData.materials.normal.type = TEX_TYPE::NORMAL;
                    }
                }
            }

            aNMeshes.push(nMesh);
        }
        this->aaMeshes.push(aNMeshes);
    }

    this->aTmIdxs = decltype(this->aTmIdxs)(this->pAlloc, sq(this->asset.aNodes.size));
    this->aTmCounters = decltype(this->aTmCounters)(this->pAlloc, this->asset.aNodes.size);

    tp.stop();
    aAlloc.freeAll();
}

void
Model::draw(enum DRAW flags, Shader* sh, adt::String svUniform, adt::String svUniformM3Norm, const m4& tmGlobal)
{
    for (auto& m : this->aaMeshes)
    {
        for (auto& e : m)
        {
            glBindVertexArray(e.meshData.vao);

            if (flags & DRAW::DIFF)
                e.meshData.materials.diffuse.bind(GL_TEXTURE0);
            if (flags & DRAW::NORM)
                e.meshData.materials.normal.bind(GL_TEXTURE1);

            m4 m = m4Iden();
            if (flags & DRAW::APPLY_TM)
                m *= tmGlobal;

            if (sh)
            {
                sh->setM4(svUniform, m);
                if (flags & DRAW::APPLY_NM) sh->setM3(svUniformM3Norm, m3Normal(m));
            }

            if (e.triangleCount != NPOS)
                glDrawArrays(static_cast<GLenum>(e.mode), 0, e.triangleCount);
            else
                glDrawElements(static_cast<GLenum>(e.mode),
                               e.meshData.eboSize,
                               static_cast<GLenum>(e.indType),
                               nullptr);
        }
    }
}

void
Model::drawGraph(enum DRAW flags,
                 Shader* sh,
                 adt::String svUniform,
                 adt::String svUniformM3Norm,
                 const m4& tmGlobal)
{
    auto& aNodes = this->asset.aNodes;
    memset(aTmIdxs.data(), 0, aTmIdxs.size * sizeof(aTmIdxs[0]));
    memset(aTmCounters.data(), 0, aTmCounters.size * sizeof(aTmIdxs[0]));

    auto at = [&](u32 r, u32 c) -> int {
        return r*aNodes.size + c;
    };

    for (u32 i = 0; i < aNodes.size; i++)
    {
        auto& node = aNodes[i];
        for (auto& ch : node.children)
            aTmIdxs[at(ch, aTmCounters[ch]++)] = i; /* give each children it's parent's idx's */

        if (node.mesh != NPOS)
        {
            m4 tm = tmGlobal;
            qt rot = qtIden();
            for (int j = 0; j < aTmCounters[i]; j++)
            {
                /* collect each transformation from parent's map */
                auto& n = aNodes[ aTmIdxs[at(i, j)] ];

                tm = m4Scale(tm, n.scale);
                rot *= n.rotation;
                tm *= n.matrix;
            }
            tm = m4Scale(tm, node.scale);
            tm *= qtRot(rot * node.rotation);
            tm = m4Translate(tm, node.translation);
            tm *= node.matrix;

            for (auto& e : this->aaMeshes[node.mesh])
            {
                glBindVertexArray(e.meshData.vao);

                if (flags & DRAW::DIFF)
                    e.meshData.materials.diffuse.bind(GL_TEXTURE0);
                if (flags & DRAW::NORM)
                    e.meshData.materials.normal.bind(GL_TEXTURE1);

                if (sh)
                {
                    sh->setM4(svUniform, tm);
                    if (flags & DRAW::APPLY_NM) sh->setM3(svUniformM3Norm, m3Normal(tm));
                }

                if (e.triangleCount != NPOS)
                    glDrawArrays(static_cast<GLenum>(e.mode), 0, e.triangleCount);
                else
                    glDrawElements(static_cast<GLenum>(e.mode),
                                   e.meshData.eboSize,
                                   static_cast<GLenum>(e.indType),
                                   nullptr);
            }
        }
    }
}

/*void*/
/*Model::drawInstanced(GLsizei count)*/
/*{*/
/*    for (auto& aaMeshes : objects)*/
/*        for (auto& mesh : aaMeshes)*/
/*        {*/
/*            glBindVertexArray(mesh.vao);*/
/*            glDrawElementsInstanced(GL_TRIANGLES, mesh.eboSize, GL_UNSIGNED_INT, nullptr, count);*/
/*        }*/
/*}*/
/**/

Ubo::Ubo(u32 _size, GLint drawMode)
{
    createBuffer(_size, drawMode);
}

void
Ubo::createBuffer(u32 _size, GLint drawMode)
{
    this->size = _size;
    glGenBuffers(1, &this->id);
    glBindBuffer(GL_UNIFORM_BUFFER, this->id);
    glBufferData(GL_UNIFORM_BUFFER, _size, nullptr, drawMode);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void
Ubo::bindBlock(Shader* sh, adt::String block, GLuint _point)
{
    this->point = _point;
    GLuint index = glGetUniformBlockIndex(sh->id, block.data());
    glUniformBlockBinding(sh->id, index, _point);
    LOG_OK("uniform block: '%.*s' at '%u', in shader '%u'\n", (int)block.size, block.pData, index, sh->id);

    glBindBufferBase(GL_UNIFORM_BUFFER, _point, this->id);
    /* or */
    // glBindBufferRange(GL_UNIFORM_BUFFER, _point, this->id, 0, size);
}

void
Ubo::bufferData(void* pData, u32 offset, u32 _size)
{
    glBindBuffer(GL_UNIFORM_BUFFER, this->id);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, _size, pData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// Model
// getQuad(GLint drawMode)
// {
//     f32 quadVertices[] {
//         -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
//         -1.0f, -1.0f,  0.0f,  0.0f,  0.0f,
//          1.0f, -1.0f,  0.0f,  1.0f,  0.0f,
//          1.0f,  1.0f,  0.0f,  1.0f,  1.0f,
//     };
// 
//     GLuint quadIndices[] {
//         0, 1, 2, 0, 2, 3
//     };
// 
//     Model q;
//     q.aaMeshes.resize(1);
//     q.aaMeshes.back().push({});
// 
//     glGenVertexArrays(1, &q.aaMeshes[0][0].meshData.vao);
//     glBindVertexArray(q.aaMeshes[0][0].meshData.vao);
// 
//     glGenBuffers(1, &q.aaMeshes[0][0].meshData.vbo);
//     glBindBuffer(GL_ARRAY_BUFFER, q.aaMeshes[0][0].meshData.vbo);
//     glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, drawMode);
// 
//     glGenBuffers(1, &q.aaMeshes[0][0].meshData.ebo);
//     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, q.aaMeshes[0][0].meshData.ebo);
//     glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, drawMode);
//     q.aaMeshes[0][0].meshData.eboSize = LEN(quadIndices);
// 
//     constexpr u32 v3Size = sizeof(v3) / sizeof(f32);
//     constexpr u32 v2Size = sizeof(v2) / sizeof(f32);
//     constexpr u32 stride = 5 * sizeof(f32);
//     /* positions */
//     glEnableVertexAttribArray(0);
//     glVertexAttribPointer(0, v3Size, GL_FLOAT, GL_FALSE, stride, (void*)0);
//     /* texture coords */
//     glEnableVertexAttribArray(1);
//     glVertexAttribPointer(1, v2Size, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(f32) * v3Size));
// 
//     glBindVertexArray(0);
// 
//     LOG_OK("quad '%u' created\n", q.aaMeshes[0][0].meshData.vao);
//     q.savedPath = "Quad";
//     return q;
// }
// 
// Model
// getPlane(GLint drawMode)
// {
//     f32 planeVertices[] {
//         /* positions            texcoords   normals          */
//          25.0f, -0.5f,  25.0f,  25.0f,  0.0f,  0.0f, 1.0f, 0.0f,  
//         -25.0f, -0.5f, -25.0f,   0.0f, 25.0f,  0.0f, 1.0f, 0.0f,  
//         -25.0f, -0.5f,  25.0f,   0.0f,  0.0f,  0.0f, 1.0f, 0.0f,  
//                                                                   
//         -25.0f, -0.5f, -25.0f,   0.0f, 25.0f,  0.0f, 1.0f, 0.0f,  
//          25.0f, -0.5f,  25.0f,  25.0f,  0.0f,  0.0f, 1.0f, 0.0f,  
//          25.0f, -0.5f, -25.0f,  25.0f, 25.0f,  0.0f, 1.0f, 0.0f 
//     };
// 
//     Model q;
//     q.aaMeshes.resize(1);
//     q.aaMeshes.back().push({});
// 
//     glGenVertexArrays(1, &q.aaMeshes[0][0].meshData.vao);
//     glBindVertexArray(q.aaMeshes[0][0].meshData.vao);
// 
//     glGenBuffers(1, &q.aaMeshes[0][0].meshData.vbo);
//     glBindBuffer(GL_ARRAY_BUFFER, q.aaMeshes[0][0].meshData.vbo);
//     glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, drawMode);
// 
//     constexpr u32 v3Size = sizeof(v3) / sizeof(f32);
//     constexpr u32 v2Size = sizeof(v2) / sizeof(f32);
//     constexpr u32 stride = 8 * sizeof(f32);
//     /* positions */
//     glEnableVertexAttribArray(0);
//     glVertexAttribPointer(0, v3Size, GL_FLOAT, GL_FALSE, stride, (void*)0);
//     /* texture coords */
//     glEnableVertexAttribArray(1);
//     glVertexAttribPointer(1, v2Size, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(f32) * v3Size));
//     /* normals */
//     glEnableVertexAttribArray(2);
//     glVertexAttribPointer(2, v3Size, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(f32) * (v3Size + v2Size)));
// 
//     glBindVertexArray(0);
// 
//     LOG_OK("plane '%u' created\n", q.aaMeshes[0][0].meshData.vao);
//     return q;
// }
// 
// void
// drawQuad(const Model& q)
// {
//     glBindVertexArray(q.aaMeshes[0][0].meshData.vao);
//     glDrawElements(GL_TRIANGLES, q.aaMeshes[0][0].meshData.eboSize, GL_UNSIGNED_INT, nullptr);
// }
// 
// void
// drawPlane(const Model& q)
// {
//     glBindVertexArray(q.aaMeshes[0][0].meshData.vao);
//     glDrawArrays(GL_TRIANGLES, 0, 6);
// }
// 
// Model
// getCube(GLint drawMode)
// {
//     float cubeVertices[] {
//         /* back face */
//         -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, /* bottom-left */
//          1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, /* top-right */
//          1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, /* bottom-right */    
//          1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, /* top-right */
//         -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, /* bottom-left */
//         -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, /* top-left */
//         /* front face */
//         -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, /* bottom-left */
//          1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, /* bottom-right */
//          1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, /* top-right */
//          1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, /* top-right */
//         -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, /* top-left */
//         -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, /* bottom-left */
//         /* left face */
//         -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, /* top-right */
//         -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, /* top-left */
//         -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, /* bottom-left */
//         -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, /* bottom-left */
//         -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, /* bottom-right */
//         -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, /* top-right */
//         /* right face */
//          1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, /* top-left */
//          1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, /* bottom-right */
//          1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, /* top-right */
//          1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, /* bottom-right */
//          1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, /* top-left */
//          1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, /* bottom-left */
//         /* bottom face */
//         -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, /* top-right */
//          1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, /* top-left */
//          1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, /* bottom-le ft */
//          1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, /* bottom-le ft */
//         -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, /* bottom-ri ght */
//         -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, /* top-right */
//         /* top face */
//         -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, /* top-left */
//          1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, /* bottom-right */
//          1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, /* top-right */
//          1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, /* bottom-right */
//         -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, /* top-left */
//         -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  /* bottom-left */
//     };
// 
//     Model q;
//     q.aaMeshes.resize(1);
// 
//     glGenVertexArrays(1, &q.aaMeshes[0][0].meshData.vao);
//     glBindVertexArray(q.aaMeshes[0][0].meshData.vao);
// 
//     glGenBuffers(1, &q.aaMeshes[0][0].meshData.vbo);
//     glBindBuffer(GL_ARRAY_BUFFER, q.aaMeshes[0][0].meshData.vbo);
//     glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, drawMode);
// 
//     constexpr u32 v3Size = sizeof(v3) / sizeof(f32);
//     constexpr u32 v2Size = sizeof(v2) / sizeof(f32);
//     constexpr u32 stride = 8 * sizeof(f32);
// 
//     /* positions */
//     glEnableVertexAttribArray(0);
//     glVertexAttribPointer(0, v3Size, GL_FLOAT, GL_FALSE, stride, (void*)0);
//     /* texture coords */
//     glEnableVertexAttribArray(1);
//     glVertexAttribPointer(1, v2Size, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(f32) * v3Size));
//     /* normals */
//     glEnableVertexAttribArray(2);
//     glVertexAttribPointer(2, v3Size, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(f32) * (v3Size + v2Size)));
// 
//     glBindVertexArray(0);
// 
//     LOG(OK, "cube '{}' created\n", q.aaMeshes[0][0].meshData.vao);
//     return q;
// }
// 
// void
// drawCube(const Model& q)
// {
//     glBindVertexArray(q.aaMeshes[0][0].meshData.vao);
//     glDrawArrays(GL_TRIANGLES, 0, 36);
// }
