#pragma once

#include "../json/parser.hh"
#include "../math.hh"
#include "utils.hh"
#include "String.hh"

namespace gltf
{

/* match gl macros */
enum class COMPONENT_TYPE
{
    BYTE = 5120,
    UNSIGNED_BYTE = 5121,
    SHORT = 5122,
    UNSIGNED_SHORT = 5123,
    UNSIGNED_INT = 5125,
    FLOAT = 5126
};

/* match gl macros */
enum class TARGET
{
    NONE = 0,
    ARRAY_BUFFER = 34962,
    ELEMENT_ARRAY_BUFFER = 34963
};

struct Scene
{
    size_t nodeIdx;
};

/* A buffer represents a block of raw binary data, without an inherent structure or meaning.
 * This data is referred to by a buffer using its uri.
 * This URI may either point to an external file, or be a data URI that encodes the binary data directly in the JSON file. */
struct Buffer
{
    size_t byteLength;
    adt::String uri;
    adt::String aBin;
};

enum class ACCESSOR_TYPE
{
    SCALAR,
    VEC2,
    VEC3,
    VEC4,
    /*MAT2, Unused*/
    MAT3,
    MAT4
};

union Type
{
    f64 SCALAR;
    v2 VEC2;
    v3 VEC3;
    v4 VEC4;
    /*m2 MAT2; Unused */
    m3 MAT3;
    m4 MAT4;
};

/* An accessor object refers to a bufferView and contains properties 
 * that define the type and layout of the data of the bufferView.
 * The raw data of a buffer is structured using bufferView objects and is augmented with data type information using accessor objects.*/
struct Accessor
{
    size_t bufferView;
    size_t byteOffset; /* The offset relative to the start of the buffer view in bytes. This MUST be a multiple of the size of the component datatype. */
    enum COMPONENT_TYPE componentType; /* REQUIRED */
    size_t count; /* (REQUIRED) The number of elements referenced by this accessor, not to be confused with the number of bytes or number of components. */
    union Type max;
    union Type min;
    enum ACCESSOR_TYPE type; /* REQUIRED */
};


/* Each node can contain an array called children that contains the indices of its child nodes.
 * So each node is one element of a hierarchy of nodes,
 * and together they define the structure of the scene as a scene graph. */
struct Node
{
    size_t camera;
    adt::Array<size_t> children;
    m4 matrix = m4Iden();
    size_t mesh = NPOS; /* The index of the mesh in this node. */
    v3 translation {};
    v4 rotation = qtIden();
    v3 scale {1, 1, 1};

    Node(adt::BaseAllocator* p) : children(p) {}
};

struct CameraPersp
{
    f64 aspectRatio;
    f64 yfov;
    f64 zfar;
    f64 znear;
};

struct CameraOrtho
{
    //
};

struct Camera
{
    union {
        CameraPersp perspective;
        CameraOrtho orthographic;
    } proj;
    enum {
        perspective,
        orthographic
    } type;
};

/* A bufferView represents a “slice” of the data of one buffer.
 * This slice is defined using an offset and a length, in bytes. */
struct BufferView
{
    size_t buffer;
    size_t byteOffset = 0; /* The offset into the buffer in bytes. */
    size_t byteLength;
    size_t byteStride = 0; /* The stride, in bytes, between vertex attributes. When this is not defined, data is tightly packed. */
    enum TARGET target;
};

struct Image
{
    adt::String uri;
};

/* match real gl macros */
enum class PRIMITIVES
{
    POINTS = 0,
    LINES = 1,
    LINE_LOOP = 2,
    LINE_STRIP = 3,
    TRIANGLES = 4,
    TRIANGLE_STRIP = 5,
    TRIANGLE_FAN = 6
};

struct Primitive
{
    struct {
        size_t NORMAL = NPOS;
        size_t POSITION = NPOS;
        size_t TEXCOORD_0 = NPOS;
        size_t TANGENT = NPOS;
    } attributes; /* each value is the index of the accessor containing attribute’s data. */
    size_t indices = NPOS; /* The index of the accessor that contains the vertex indices, drawElements() when defined and drawArrays() otherwise. */
    size_t material = NPOS; /* The index of the material to apply to this primitive when rendering */
    enum PRIMITIVES mode = PRIMITIVES::TRIANGLES;
};

/* A mesh primitive defines the geometry data of the object using its attributes dictionary.
 * This geometry data is given by references to accessor objects that contain the data of vertex attributes. */
struct Mesh
{
    adt::Array<Primitive> aPrimitives; /* REQUIRED */
    adt::String svName;
};

struct Texture
{
    size_t source = NPOS; /* The index of the image used by this texture. */
    size_t sampler = NPOS; /* The index of the sampler used by this texture. When undefined, a sampler with repeat wrapping and auto filtering SHOULD be used. */
};

struct TextureInfo
{
    size_t index = NPOS; /* (REQUIRED) The index of the texture. */
};

struct NormalTextureInfo
{
    size_t index = NPOS; /* (REQUIRED) */
    f64 scale;
};

struct PbrMetallicRoughness
{
    TextureInfo baseColorTexture;
};

struct Material
{
    PbrMetallicRoughness pbrMetallicRoughness;
    NormalTextureInfo normalTexture;
};

struct Asset
{
    adt::BaseAllocator* pAlloc;
    json::Parser parser;
    adt::String svGenerator;
    adt::String svVersion;
    size_t defaultSceneIdx;
    adt::Array<Scene> aScenes;
    adt::Array<Buffer> aBuffers;
    adt::Array<BufferView> aBufferViews;
    adt::Array<Accessor> aAccessors;
    adt::Array<Mesh> aMeshes;
    adt::Array<Texture> aTextures;
    adt::Array<Material> aMaterials;
    adt::Array<Image> aImages;
    adt::Array<Node> aNodes;

    Asset(adt::BaseAllocator* p)
        : pAlloc(p), parser(p), aScenes(p), aBuffers(p), aBufferViews(p), aAccessors(p), aMeshes(p), aTextures(p), aMaterials(p), aImages(p), aNodes(p) {}
    Asset(adt::BaseAllocator* p, adt::String path)
        : pAlloc(p), parser(p), aScenes(p), aBuffers(p), aBufferViews(p), aAccessors(p), aMeshes(p), aTextures(p), aMaterials(p), aImages(p), aNodes(p)
        { this->load(path); }

    void load(adt::String path);
private:
    struct {
        json::Object* scene;
        json::Object* scenes;
        json::Object* nodes;
        json::Object* meshes;
        json::Object* cameras;
        json::Object* buffers;
        json::Object* bufferViews;
        json::Object* accessors;
        json::Object* materials;
        json::Object* textures;
        json::Object* images;
        json::Object* samplers;
        json::Object* skins;
        json::Object* animations;
    } jsonObjs {};

    void processJSONObjs();
    void processScenes();
    void processBuffers();
    void processBufferViews();
    void processAccessors();
    void processMeshes();
    void processTexures();
    void processMaterials();
    void processImages();
    void processNodes();
};

static inline adt::String
getComponentTypeString(enum COMPONENT_TYPE t)
{
    switch (t)
    {
        default:
        case COMPONENT_TYPE::BYTE:
            return "BYTE";
        case COMPONENT_TYPE::UNSIGNED_BYTE:
            return "UNSIGNED_BYTE";
        case COMPONENT_TYPE::SHORT:
            return "SHORT";
        case COMPONENT_TYPE::UNSIGNED_SHORT:
            return "UNSIGNED_SHORT";
        case COMPONENT_TYPE::UNSIGNED_INT:
            return "UNSIGNED_INT";
        case COMPONENT_TYPE::FLOAT:
            return "FLOAT";
    }
}

static inline adt::String
getPrimitiveModeString(enum PRIMITIVES pm)
{
    const char* ss[] {
        "POINTS", "LINES", "LINE_LOOP", "LINE_STRIP", "TRIANGLES", "TRIANGLE_STRIP", "TRIANGLE_FAN"
    };

    return ss[static_cast<int>(pm)];
}

} /* namespace gltf */
