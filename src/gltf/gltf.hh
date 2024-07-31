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
    u32 nodeIdx;
};

/* A buffer represents a block of raw binary data, without an inherent structure or meaning.
 * This data is referred to by a buffer using its uri.
 * This URI may either point to an external file, or be a data URI that encodes the binary data directly in the JSON file. */
struct Buffer
{
    u32 byteLength;
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
    u32 bufferView;
    u32 byteOffset; /* The offset relative to the start of the buffer view in bytes. This MUST be a multiple of the size of the component datatype. */
    enum COMPONENT_TYPE componentType; /* REQUIRED */
    u32 count; /* (REQUIRED) The number of elements referenced by this accessor, not to be confused with the number of bytes or number of components. */
    union Type max;
    union Type min;
    enum ACCESSOR_TYPE type; /* REQUIRED */
};


/* Each node can contain an array called children that contains the indices of its child nodes.
 * So each node is one element of a hierarchy of nodes,
 * and together they define the structure of the scene as a scene graph. */
struct Node
{
    adt::String name;
    u32 camera;
    adt::Array<u32> children;
    m4 matrix = m4Iden();
    u32 mesh = adt::NPOS; /* The index of the mesh in this node. */
    v3 translation {};
    v4 rotation = qtIden();
    v3 scale {1, 1, 1};

    Node() = default;
    Node(adt::Allocator* p) : children(p) {}
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
    u32 buffer;
    u32 byteOffset = 0; /* The offset into the buffer in bytes. */
    u32 byteLength;
    u32 byteStride = 0; /* The stride, in bytes, between vertex attributes. When this is not defined, data is tightly packed. */
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
        u32 NORMAL = adt::NPOS;
        u32 POSITION = adt::NPOS;
        u32 TEXCOORD_0 = adt::NPOS;
        u32 TANGENT = adt::NPOS;
    } attributes; /* each value is the index of the accessor containing attribute’s data. */
    u32 indices = adt::NPOS; /* The index of the accessor that contains the vertex indices, drawElements() when defined and drawArrays() otherwise. */
    u32 material = adt::NPOS; /* The index of the material to apply to this primitive when rendering */
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
    u32 source = adt::NPOS; /* The index of the image used by this texture. */
    u32 sampler = adt::NPOS; /* The index of the sampler used by this texture. When undefined, a sampler with repeat wrapping and auto filtering SHOULD be used. */
};

struct TextureInfo
{
    u32 index = adt::NPOS; /* (REQUIRED) The index of the texture. */
};

struct NormalTextureInfo
{
    u32 index = adt::NPOS; /* (REQUIRED) */
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
    adt::Allocator* _pAlloc;
    json::Parser _parser;
    adt::String _svGenerator;
    adt::String _svVersion;
    u32 _defaultSceneIdx;
    adt::Array<Scene> _aScenes;
    adt::Array<Buffer> _aBuffers;
    adt::Array<BufferView> _aBufferViews;
    adt::Array<Accessor> _aAccessors;
    adt::Array<Mesh> _aMeshes;
    adt::Array<Texture> _aTextures;
    adt::Array<Material> _aMaterials;
    adt::Array<Image> _aImages;
    adt::Array<Node> _aNodes;

    Asset(adt::Allocator* p)
        : _pAlloc(p), _parser(p), _aScenes(p), _aBuffers(p), _aBufferViews(p), _aAccessors(p), _aMeshes(p), _aTextures(p), _aMaterials(p), _aImages(p), _aNodes(p) {}
    Asset(adt::Allocator* p, adt::String path)
        : Asset(p) { this->load(path); }

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
    } _jsonObjs {};

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

inline adt::String
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

inline adt::String
getPrimitiveModeString(enum PRIMITIVES pm)
{
    const char* ss[] {
        "POINTS", "LINES", "LINE_LOOP", "LINE_STRIP", "TRIANGLES", "TRIANGLE_STRIP", "TRIANGLE_FAN"
    };

    return ss[int(pm)];
}

} /* namespace gltf */
