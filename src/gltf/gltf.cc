#include "ArenaAllocator.hh"
#include "ThreadPool.hh"
#include "file.hh"
#include "gltf.hh"
#include "logs.hh"

namespace gltf
{

enum class HASH_CODES : u64
{
    scene = adt::hashFNV("scene"),
    scenes = adt::hashFNV("scenes"),
    nodes = adt::hashFNV("nodes"),
    meshes = adt::hashFNV("meshes"),
    cameras = adt::hashFNV("cameras"),
    buffers = adt::hashFNV("buffers"),
    bufferViews = adt::hashFNV("bufferViews"),
    accessors = adt::hashFNV("accessors"),
    materials = adt::hashFNV("materials"),
    textures = adt::hashFNV("textures"),
    images = adt::hashFNV("images"),
    samplers = adt::hashFNV("samplers"),
    skins = adt::hashFNV("skins"),
    animations = adt::hashFNV("animations"),
    SCALAR = adt::hashFNV("SCALAR"),
    VEC2 = adt::hashFNV("VEC2"),
    VEC3 = adt::hashFNV("VEC3"),
    VEC4 = adt::hashFNV("VEC4"),
    MAT3 = adt::hashFNV("MAT3"),
    MAT4 = adt::hashFNV("MAT4")
};

#ifdef GLTF

static adt::String
getTargetString(enum TARGET t)
{
    switch (t)
    {
        default:
        case TARGET::NONE:
            return "NONE";
        case TARGET::ARRAY_BUFFER:
            return "ARRAY_BUFFER";
        case TARGET::ELEMENT_ARRAY_BUFFER:
            return "ELEMENT_ARRAY_BUFFER";
    }
}

static adt::String
accessorTypeToString(enum ACCESSOR_TYPE t)
{
    const char* ss[] {
        "SCALAR", "VEC2", "VEC3", "VEC4", /*MAT2, Unused*/ "MAT3", "MAT4"
    };
    return ss[static_cast<int>(t)];
}

#endif

inline enum ACCESSOR_TYPE
stringToAccessorType(adt::String sv)
{
    switch (adt::hashFNV(sv))
    {
        default:
        case static_cast<u64>(HASH_CODES::SCALAR):
            return ACCESSOR_TYPE::SCALAR;
        case static_cast<u64>(HASH_CODES::VEC2):
            return ACCESSOR_TYPE::VEC2;
        case static_cast<u64>(HASH_CODES::VEC3):
            return ACCESSOR_TYPE::VEC3;
        case static_cast<u64>(HASH_CODES::VEC4):
            return ACCESSOR_TYPE::VEC4;
        case static_cast<u64>(HASH_CODES::MAT3):
            return ACCESSOR_TYPE::MAT3;
        case static_cast<u64>(HASH_CODES::MAT4):
            return ACCESSOR_TYPE::MAT4;
    }
}

inline union Type
assignUnionType(json::Object* obj, u32 n)
{
    auto& arr = json::getArray(obj);
    union Type type;

    for (u32 i = 0; i < n; i++)
        if (arr[i].tagVal.tag == json::TAG::LONG)
            type.MAT4.p[i] = f32(json::getLong(&arr[i]));
        else
            type.MAT4.p[i] = f32(json::getDouble(&arr[i]));

    return type;
}

static union Type
accessorTypeToUnionType(enum ACCESSOR_TYPE t, json::Object* obj)
{
    union Type type;

    switch (t)
    {
        default:
        case ACCESSOR_TYPE::SCALAR:
            {
                auto& arr = json::getArray(obj);
                if (arr[0].tagVal.tag == json::TAG::LONG)
                    type.SCALAR = f64(json::getLong(&arr[0]));
                else
                    type.SCALAR = f64(json::getDouble(&arr[0]));
            }
            break;
        case ACCESSOR_TYPE::VEC2:
            type = assignUnionType(obj, 2);
            break;
        case ACCESSOR_TYPE::VEC3:
            type = assignUnionType(obj, 3);
            break;
        case ACCESSOR_TYPE::VEC4:
            type = assignUnionType(obj, 4);
            break;
        case ACCESSOR_TYPE::MAT3:
            type = assignUnionType(obj, 3*3);
            break;
        case ACCESSOR_TYPE::MAT4:
            type = assignUnionType(obj, 4*4);
            break;
    }

    return type;
}

void
Asset::load(adt::String path)
{
    _parser.load(path);
    _parser.parse();

    processJSONObjs();
    _defaultSceneIdx = json::getLong(this->_jsonObjs.scene);

    adt::ArenaAllocator alloc(adt::SIZE_1K);
    adt::ThreadPool tp(&alloc, 9);
    tp.start();

    tp.submit([](void* a) { ((Asset*)a)->processScenes();      return 0; }, this);
    tp.submit([](void* a) { ((Asset*)a)->processBuffers();     return 0; }, this);
    tp.submit([](void* a) { ((Asset*)a)->processBufferViews(); return 0; }, this);
    tp.submit([](void* a) { ((Asset*)a)->processAccessors();   return 0; }, this);
    tp.submit([](void* a) { ((Asset*)a)->processMeshes();      return 0; }, this);
    tp.submit([](void* a) { ((Asset*)a)->processTexures();     return 0; }, this);
    tp.submit([](void* a) { ((Asset*)a)->processMaterials();   return 0; }, this);
    tp.submit([](void* a) { ((Asset*)a)->processImages();      return 0; }, this);
    tp.submit([](void* a) { ((Asset*)a)->processNodes();       return 0; }, this);

    tp.wait();
    tp.destroy();
    alloc.freeAll();
}

void
Asset::processJSONObjs()
{
    /* collect all the top level objects */
    for (auto& node : json::getObject(_parser.getHeadObj()))
    {
        switch (adt::hashFNV(node.svKey))
        {
            default:
                break;
            case static_cast<u64>(HASH_CODES::scene):
                _jsonObjs.scene = &node;
                break;
            case static_cast<u64>(HASH_CODES::scenes):
                _jsonObjs.scenes = &node;
                break;
            case static_cast<u64>(HASH_CODES::nodes):
                _jsonObjs.nodes = &node;
                break;
            case static_cast<u64>(HASH_CODES::meshes):
                _jsonObjs.meshes = &node;
                break;
            case static_cast<u64>(HASH_CODES::cameras):
                _jsonObjs.cameras = &node;
                break;
            case static_cast<u64>(HASH_CODES::buffers):
                _jsonObjs.buffers = &node;
                break;
            case static_cast<u64>(HASH_CODES::bufferViews):
                _jsonObjs.bufferViews = &node;
                break;
            case static_cast<u64>(HASH_CODES::accessors):
                _jsonObjs.accessors = &node;
                break;
            case static_cast<u64>(HASH_CODES::materials):
                _jsonObjs.materials = &node;
                break;
            case static_cast<u64>(HASH_CODES::textures):
                _jsonObjs.textures = &node;
                break;
            case static_cast<u64>(HASH_CODES::images):
                _jsonObjs.images = &node;
                break;
            case static_cast<u64>(HASH_CODES::samplers):
                _jsonObjs.samplers = &node;
                break;
            case static_cast<u64>(HASH_CODES::skins):
                _jsonObjs.skins = &node;
                break;
            case static_cast<u64>(HASH_CODES::animations):
                _jsonObjs.animations = &node;
                break;
        }
    }

#ifdef GLTF
    LOG_OK("GLTF: '%.*s'\n", parser.sName.size, this->parser.sName.pData);
    auto check = [](adt::String sv, json::Object* p) -> void {
        adt::String s = p ? p->svKey : "(null)";
        CERR("\t%.*s: '%.*s'\n", sv.size, sv.data(), s.size, s.pData);
    };

    check("scene", jsonObjs.scene);
    check("scenes", jsonObjs.scenes);
    check("nodes", jsonObjs.nodes);
    check("meshes", jsonObjs.meshes);
    check("buffers", jsonObjs.buffers);
    check("bufferViews", jsonObjs.bufferViews);
    check("accessors", jsonObjs.accessors);
    check("materials", jsonObjs.materials);
    check("textures", jsonObjs.textures);
    check("images", jsonObjs.images);
    check("samplers", jsonObjs.samplers);
    check("skins", jsonObjs.skins);
    check("animations", jsonObjs.animations);
#endif
}

void
Asset::processScenes()
{
    auto scenes = _jsonObjs.scenes;
    auto& arr = json::getArray(scenes);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);
        auto pNodes = json::searchObject(obj, "nodes");
        if (pNodes)
        {
            auto& a = json::getArray(pNodes);
            for (auto& el : a)
                _aScenes.push({(u32)json::getLong(&el)});
        }
        else
        {
            _aScenes.push({0});
            break;
        }
    }
}

void
Asset::processBuffers()
{
    auto buffers = _jsonObjs.buffers;
    auto& arr = json::getArray(buffers);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);
        auto pByteLength = json::searchObject(obj, "byteLength");
        auto pUri = json::searchObject(obj, "uri");
        if (!pByteLength) LOG_FATAL("'byteLength' field is required\n");

        adt::String svUri;
        adt::String aBin;

        if (pUri)
        {
            svUri = json::getString(pUri);
            auto sNewPath = adt::replacePathSuffix(_pAlloc, this->_parser._sName, svUri);
            aBin = adt::loadFile(_pAlloc, sNewPath);
        }

        _aBuffers.push({
            .byteLength = static_cast<u32>(json::getLong(pByteLength)),
            .uri = svUri,
            .aBin = aBin
        });
    }
}

void
Asset::processBufferViews()
{
    auto bufferViews = _jsonObjs.bufferViews;
    auto& arr = json::getArray(bufferViews);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);

        auto pBuffer = json::searchObject(obj, "buffer");
        if (!pBuffer) LOG_FATAL("'buffer' field is required\n");
        auto pByteOffset = json::searchObject(obj, "byteOffset");
        auto pByteLength = json::searchObject(obj, "byteLength");
        if (!pByteLength) LOG_FATAL("'byteLength' field is required\n");
        auto pByteStride = json::searchObject(obj, "byteStride");
        auto pTarget = json::searchObject(obj, "target");

        _aBufferViews.push({
            .buffer = static_cast<u32>(json::getLong(pBuffer)),
            .byteOffset = pByteOffset ? static_cast<u32>(json::getLong(pByteOffset)) : 0,
            .byteLength = static_cast<u32>(json::getLong(pByteLength)),
            .byteStride = pByteStride ? static_cast<u32>(json::getLong(pByteStride)) : 0,
            .target = pTarget ? static_cast<enum TARGET>(json::getLong(pTarget)) : TARGET::NONE
        });
    }
}

void
Asset::processAccessors()
{
    auto accessors = _jsonObjs.accessors;
    auto& arr = json::getArray(accessors);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);
 
        auto pBufferView = json::searchObject(obj, "bufferView");
        auto pByteOffset = json::searchObject(obj, "byteOffset");
        auto pComponentType = json::searchObject(obj, "componentType");
        if (!pComponentType) LOG_FATAL("'componentType' field is required\n");
        auto pCount = json::searchObject(obj, "count");
        if (!pCount) LOG_FATAL("'count' field is required\n");
        auto pMax = json::searchObject(obj, "max");
        auto pMin = json::searchObject(obj, "min");
        auto pType = json::searchObject(obj, "type");
        if (!pType) LOG_FATAL("'type' field is required\n");
 
        enum ACCESSOR_TYPE type = stringToAccessorType(json::getString(pType));
 
        _aAccessors.push({
            .bufferView = pBufferView ? static_cast<u32>(json::getLong(pBufferView)) : 0,
            .byteOffset = pByteOffset ? static_cast<u32>(json::getLong(pByteOffset)) : 0,
            .componentType = static_cast<enum COMPONENT_TYPE>(json::getLong(pComponentType)),
            .count = static_cast<u32>(json::getLong(pCount)),
            .max = pMax ? accessorTypeToUnionType(type, pMax) : Type{},
            .min = pMin ? accessorTypeToUnionType(type, pMin) : Type{},
            .type = type
        });
    }
}

void
Asset::processMeshes()
{
    auto meshes = _jsonObjs.meshes;
    auto& arr = json::getArray(meshes);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);
 
        auto pPrimitives = json::searchObject(obj, "primitives");
        if (!pPrimitives) LOG_FATAL("'primitives' field is required\n");
 
        adt::Array<Primitive> aPrimitives(_pAlloc);
        auto pName = json::searchObject(obj, "name");
        auto name = pName ? json::getString(pName) : "";
 
        auto& aPrim = json::getArray(pPrimitives);
        for (auto& p : aPrim)
        {
            auto& op = json::getObject(&p);
 
            auto pAttributes = json::searchObject(op, "attributes");
            auto& oAttr = json::getObject(pAttributes);
            auto pNORMAL = json::searchObject(oAttr, "NORMAL");
            auto pTANGENT = json::searchObject(oAttr, "TANGENT");
            auto pPOSITION = json::searchObject(oAttr, "POSITION");
            auto pTEXCOORD_0 = json::searchObject(oAttr, "TEXCOORD_0");
 
            auto pIndices = json::searchObject(op, "indices");
            auto pMode = json::searchObject(op, "mode");
            auto pMaterial = json::searchObject(op, "material");
 
            aPrimitives.push({
                .attributes {
                    .NORMAL = pNORMAL ? static_cast<decltype(Primitive::attributes.NORMAL)>(json::getLong(pNORMAL)) : adt::NPOS,
                    .POSITION = pPOSITION ? static_cast<decltype(Primitive::attributes.POSITION)>(json::getLong(pPOSITION)) : adt::NPOS,
                    .TEXCOORD_0 = pTEXCOORD_0 ? static_cast<decltype(Primitive::attributes.TEXCOORD_0)>(json::getLong(pTEXCOORD_0)) : adt::NPOS,
                    .TANGENT = pTANGENT ? static_cast<decltype(Primitive::attributes.TANGENT)>(json::getLong(pTANGENT)) : adt::NPOS,
                },
                .indices = pIndices ? static_cast<decltype(Primitive::indices)>(json::getLong(pIndices)) : adt::NPOS,
                .material = pMaterial ? static_cast<decltype(Primitive::material)>(json::getLong(pMaterial)) : adt::NPOS,
                .mode = pMode ? static_cast<decltype(Primitive::mode)>(json::getLong(pMode)) : PRIMITIVES::TRIANGLES,
            });
        }
 
        _aMeshes.push({.aPrimitives = aPrimitives, .svName = name});
    }
}

void
Asset::processTexures()
{
    auto textures = _jsonObjs.textures;
    if (!textures) return;

    auto& arr = json::getArray(textures);
    for (auto& tex : arr)
    {
        auto& obj = json::getObject(&tex);

        auto pSource = json::searchObject(obj, "source");
        auto pSampler = json::searchObject(obj, "sampler");

        _aTextures.push({
            .source = pSource ? static_cast<u32>(json::getLong(pSource)) : adt::NPOS,
            .sampler = pSampler ? static_cast<u32>(json::getLong(pSampler)) : adt::NPOS
        });
    }
}

void
Asset::processMaterials()
{
    auto materials = _jsonObjs.materials;
    if (!materials) return;

    auto& arr = json::getArray(materials);
    for (auto& mat : arr)
    {
        auto& obj = json::getObject(&mat);

        TextureInfo texInfo {};

        auto pPbrMetallicRoughness = json::searchObject(obj, "pbrMetallicRoughness");
        if (pPbrMetallicRoughness)
        {
            auto& oPbr = json::getObject(pPbrMetallicRoughness);

            auto pBaseColorTexture = json::searchObject(oPbr, "baseColorTexture");
            if (pBaseColorTexture)
            {
                auto& objBct = json::getObject(pBaseColorTexture);

                auto pIndex = json::searchObject(objBct, "index");
                if (!pIndex) LOG_FATAL("index field is required\n");

                texInfo.index = json::getLong(pIndex);
            }
        }

        NormalTextureInfo normTexInfo {};

        auto pNormalTexture = json::searchObject(obj, "normalTexture");
        if (pNormalTexture)
        {
            auto& objNT = json::getObject(pNormalTexture);
            auto pIndex = json::searchObject(objNT, "index");
            if (!pIndex) LOG_FATAL("index filed is required\n");

            normTexInfo.index = json::getLong(pIndex);
        }

        _aMaterials.push({
            .pbrMetallicRoughness {
                .baseColorTexture = texInfo,
            },
            .normalTexture = normTexInfo
        });
    }
}

void
Asset::processImages()
{
    auto imgs = _jsonObjs.images;
    if (!imgs) return;

    auto& arr = json::getArray(imgs);
    for (auto& img : arr)
    {
        auto& obj = json::getObject(&img);

        auto pUri = json::searchObject(obj, "uri");
        if (pUri)
            _aImages.push({json::getString(pUri)});
    }
}

void
Asset::processNodes()
{
    auto nodes = _jsonObjs.nodes;
    auto& arr = json::getArray(nodes);
    for (auto& node : arr)
    {
        auto& obj = json::getObject(&node);

        Node nNode(_pAlloc);

        auto pName = json::searchObject(obj, "name");
        if (pName) nNode.name = json::getString(pName);

        auto pCamera = json::searchObject(obj, "camera");
        if (pCamera) nNode.camera = static_cast<u32>(json::getLong(pCamera));

        auto pChildren = json::searchObject(obj, "children");
        if (pChildren)
        {
            auto& arrChil = json::getArray(pChildren);
            for (auto& c : arrChil)
                nNode.children.push(static_cast<u32>(json::getLong(&c)));
        }

        auto pMatrix = json::searchObject(obj, "matrix");
        if (pMatrix)
        {
            auto ut = assignUnionType(pMatrix, 4*4);
            nNode.matrix = ut.MAT4;
        }

        auto pMesh = json::searchObject(obj, "mesh");
        if (pMesh) nNode.mesh = static_cast<u32>(json::getLong(pMesh));

        auto pTranslation = json::searchObject(obj, "translation");
        if (pTranslation)
        {
            auto ut = assignUnionType(pTranslation, 3);
            nNode.translation = ut.VEC3;
        }

        auto pRotation = json::searchObject(obj, "rotation");
        if (pRotation)
        {
            auto ut = assignUnionType(pRotation, 4);
            nNode.rotation = ut.VEC4;
        }

        auto pScale = json::searchObject(obj, "scale");
        if (pScale)
        {
            auto ut = assignUnionType(pScale, 3);
            nNode.scale = ut.VEC3;
        }

        _aNodes.push(nNode);
    }
}

} /* namespace gltf */
