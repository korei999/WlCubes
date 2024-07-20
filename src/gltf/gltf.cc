#include "gltf.hh"
#include "logs.hh"
#include "file.hh"

namespace gltf
{

enum class HASH_CODES : u64
{
    scene = adt::fnHash("scene"),
    scenes = adt::fnHash("scenes"),
    nodes = adt::fnHash("nodes"),
    meshes = adt::fnHash("meshes"),
    cameras = adt::fnHash("cameras"),
    buffers = adt::fnHash("buffers"),
    bufferViews = adt::fnHash("bufferViews"),
    accessors = adt::fnHash("accessors"),
    materials = adt::fnHash("materials"),
    textures = adt::fnHash("textures"),
    images = adt::fnHash("images"),
    samplers = adt::fnHash("samplers"),
    skins = adt::fnHash("skins"),
    animations = adt::fnHash("animations"),
    SCALAR = adt::fnHash("SCALAR"),
    VEC2 = adt::fnHash("VEC2"),
    VEC3 = adt::fnHash("VEC3"),
    VEC4 = adt::fnHash("VEC4"),
    MAT3 = adt::fnHash("MAT3"),
    MAT4 = adt::fnHash("MAT4")
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

// static std::string
// getUnionTypeString(enum ACCESSOR_TYPE type, const union Type& t, adt::String prefix)
// {
//     switch (type)
//     {
//         default:
//             return "unknown";
//         case ACCESSOR_TYPE::SCALAR:
//             return FMT("{}({})", prefix, t.SCALAR);
//         case ACCESSOR_TYPE::VEC2:
//             return FMT("{}({}, {})", prefix, t.VEC2.x, t.VEC2.y);
//         case ACCESSOR_TYPE::VEC3:
//             return FMT("{}({}, {}, {})", prefix, t.VEC3.x, t.VEC3.y, t.VEC3.z);
//         case ACCESSOR_TYPE::VEC4:
//             return FMT("{}({}, {}, {}, {})", prefix, t.VEC4.x, t.VEC4.y, t.VEC4.z, t.VEC4.w);
//         case ACCESSOR_TYPE::MAT3:
//             return FMT("{}({}, {}, {}\n"
//                        "{} {}, {}, {}\n"
//                        "{} {}, {}, {})\n", prefix, t.MAT3.e[0][0], t.MAT3.e[0][1], t.MAT3.e[0][2],
//                                            prefix, t.MAT3.e[1][0], t.MAT3.e[1][1], t.MAT3.e[1][2],
//                                            prefix, t.MAT3.e[2][0], t.MAT3.e[2][1], t.MAT3.e[2][2]);
//         case ACCESSOR_TYPE::MAT4:
//             return FMT("{}({}, {}, {}, {}\n"
//                        "{} {}, {}, {}, {}\n"
//                        "{} {}, {}, {}, {}\n"
//                        "{} {}, {}, {}, {})\n", prefix, t.MAT4.e[0][0], t.MAT4.e[0][1], t.MAT4.e[0][2], t.MAT4.e[0][3],
//                                                prefix, t.MAT4.e[1][0], t.MAT4.e[1][1], t.MAT4.e[1][2], t.MAT4.e[1][3],
//                                                prefix, t.MAT4.e[2][0], t.MAT4.e[2][1], t.MAT4.e[2][2], t.MAT4.e[2][3],
//                                                prefix, t.MAT4.e[3][0], t.MAT4.e[3][1], t.MAT4.e[3][2], t.MAT4.e[3][3]);
// 
//     }
// }

static adt::String
accessorTypeToString(enum ACCESSOR_TYPE t)
{
    const char* ss[] {
        "SCALAR", "VEC2", "VEC3", "VEC4", /*MAT2, Unused*/ "MAT3", "MAT4"
    };
    return ss[static_cast<int>(t)];
}

#endif

static inline enum ACCESSOR_TYPE
stringToAccessorType(adt::String sv)
{
    switch (adt::fnHash(sv))
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

static inline union Type
assignUnionType(json::Object* obj, u32 n)
{
    auto& arr = json::getArray(obj);
    union Type type;

    for (u32 i = 0; i < n; i++)
        if (arr[i].tagVal.tag == json::TAG::LONG)
            type.MAT4.p[i] = static_cast<f64>(json::getLong(&arr[i]));
        else
            type.MAT4.p[i] = json::getDouble(&arr[i]);

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
                    type.SCALAR = static_cast<f64>(json::getLong(&arr[0]));
                else
                    type.SCALAR = static_cast<f64>(json::getDouble(&arr[0]));
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
    this->parser.load(path);
    this->parser.parse();

    this->processJSONObjs();
    this->defaultSceneIdx = json::getLong(this->jsonObjs.scene);

    /*ThreadPool tp(std::thread::hardware_concurrency());*/

    this->processScenes();
    this->processBuffers();
    this->processBufferViews();
    this->processAccessors();
    this->processMeshes();
    this->processTexures();
    this->processMaterials();
    this->processImages();
    this->processNodes();

    /*tp.wait();*/
}

void
Asset::processJSONObjs()
{
    /* collect all the top level objects */
    for (auto& node : json::getObject(this->parser.pHead))
    {
        switch (adt::fnHash(node.svKey))
        {
            default:
                break;
            case static_cast<u64>(HASH_CODES::scene):
                this->jsonObjs.scene = &node;
                break;
            case static_cast<u64>(HASH_CODES::scenes):
                this->jsonObjs.scenes = &node;
                break;
            case static_cast<u64>(HASH_CODES::nodes):
                this->jsonObjs.nodes = &node;
                break;
            case static_cast<u64>(HASH_CODES::meshes):
                this->jsonObjs.meshes = &node;
                break;
            case static_cast<u64>(HASH_CODES::cameras):
                this->jsonObjs.cameras = &node;
                break;
            case static_cast<u64>(HASH_CODES::buffers):
                this->jsonObjs.buffers = &node;
                break;
            case static_cast<u64>(HASH_CODES::bufferViews):
                this->jsonObjs.bufferViews = &node;
                break;
            case static_cast<u64>(HASH_CODES::accessors):
                this->jsonObjs.accessors = &node;
                break;
            case static_cast<u64>(HASH_CODES::materials):
                this->jsonObjs.materials = &node;
                break;
            case static_cast<u64>(HASH_CODES::textures):
                this->jsonObjs.textures = &node;
                break;
            case static_cast<u64>(HASH_CODES::images):
                this->jsonObjs.images = &node;
                break;
            case static_cast<u64>(HASH_CODES::samplers):
                this->jsonObjs.samplers = &node;
                break;
            case static_cast<u64>(HASH_CODES::skins):
                this->jsonObjs.skins = &node;
                break;
            case static_cast<u64>(HASH_CODES::animations):
                this->jsonObjs.animations = &node;
                break;
        }
    }

#ifdef GLTF
    LOG_OK("GLTF: '%.*s'\n", (int)this->parser.sName, this->parser.sName.pData);
    auto check = [](adt::String sv, json::Object* p) -> void {
        CERR("\t{}: '{}'\n", sv, p ? p->svKey : "(null)");
    };

    check("scene", this->jsonObjs.scene);
    check("scenes", this->jsonObjs.scenes);
    check("nodes", this->jsonObjs.nodes);
    check("meshes", this->jsonObjs.meshes);
    check("buffers", this->jsonObjs.buffers);
    check("bufferViews", this->jsonObjs.bufferViews);
    check("accessors", this->jsonObjs.accessors);
    check("materials", this->jsonObjs.materials);
    check("textures", this->jsonObjs.textures);
    check("images", this->jsonObjs.images);
    check("samplers", this->jsonObjs.samplers);
    check("skins", this->jsonObjs.skins);
    check("animations", this->jsonObjs.animations);
#endif
}

void
Asset::processScenes()
{
    auto scenes = this->jsonObjs.scenes;
    auto& arr = json::getArray(scenes);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);
        auto pNodes = json::searchObject(obj, "nodes");
        if (pNodes)
        {
            auto& a = json::getArray(pNodes);
            for (auto& el : a)
                this->aScenes.push({(u32)json::getLong(&el)});
        }
        else
        {
            this->aScenes.push({0});
            break;
        }
    }

#ifdef GLTF
    // LOG(OK, "scene nodes: ");
    // for (auto& n : this->aScenes)
    //     CERR("{}, ", n.nodeIdx);
    // CERR("\n");
#endif
}

void
Asset::processBuffers()
{
    auto buffers = this->jsonObjs.buffers;
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
            svUri = json::getStringView(pUri);
            auto sNewPath = adt::replacePathSuffix(this->pAlloc, this->parser.sName, svUri);
            aBin = adt::loadFile(this->pAlloc, sNewPath);
        }

        this->aBuffers.push({
            .byteLength = static_cast<u32>(json::getLong(pByteLength)),
            .uri = svUri,
            .aBin = aBin
        });
    }

#ifdef GLTF
    // LOG(OK, "buffers:\n");
    // for (auto& b : this->aBuffers)
    //     CERR("\tbyteLength: '{}', uri: '{}'\n", b.byteLength, b.uri);
#endif
}

void
Asset::processBufferViews()
{
    auto bufferViews = this->jsonObjs.bufferViews;
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

        this->aBufferViews.push({
            .buffer = static_cast<u32>(json::getLong(pBuffer)),
            .byteOffset = pByteOffset ? static_cast<u32>(json::getLong(pByteOffset)) : 0,
            .byteLength = static_cast<u32>(json::getLong(pByteLength)),
            .byteStride = pByteStride ? static_cast<u32>(json::getLong(pByteStride)) : 0,
            .target = pTarget ? static_cast<enum TARGET>(json::getLong(pTarget)) : TARGET::NONE
        });
    }

#ifdef GLTF
    // LOG(OK, "bufferViews:\n");
    // for (auto& bv : this->aBufferViews)
    //     CERR("\tbuffer: '{}'\n\tbyteOffset: '{}'\n\tbyteLength: '{}'\n\tbyteStride: '{}'\n\ttarget: '{}'\n\n",
    //          bv.buffer, bv.byteOffset, bv.byteLength, bv.byteStride, getTargetString(bv.target));
#endif
}

void
Asset::processAccessors()
{
    auto accessors = this->jsonObjs.accessors;
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
 
        enum ACCESSOR_TYPE type = stringToAccessorType(json::getStringView(pType));
 
        this->aAccessors.push({
            .bufferView = pBufferView ? static_cast<u32>(json::getLong(pBufferView)) : 0,
            .byteOffset = pByteOffset ? static_cast<u32>(json::getLong(pByteOffset)) : 0,
            .componentType = static_cast<enum COMPONENT_TYPE>(json::getLong(pComponentType)),
            .count = static_cast<u32>(json::getLong(pCount)),
            .max = pMax ? accessorTypeToUnionType(type, pMax) : Type{},
            .min = pMin ? accessorTypeToUnionType(type, pMin) : Type{},
            .type = type
        });
    }

#ifdef GLTF
    // LOG(OK, "accessors:\n");
    // for (auto& a : this->aAccessors)
    // {
    //     CERR("\tbufferView: '{}'\n\tbyteOffset: '{}'\n\tcomponentType: '{}'\n\tcount: '{}'\n",
    //          a.bufferView, a.byteOffset, getComponentTypeString(a.componentType), a.count);
    //     CERR("\tmax:\n{}\n", getUnionTypeString(a.type, a.max, "\t"));
    //     CERR("\tmin:\n{}\n", getUnionTypeString(a.type, a.min, "\t"));
    //     CERR("\ttype: '{}'\n\n", accessorTypeToString(a.type));
    // }
#endif
}

void
Asset::processMeshes()
{
    auto meshes = this->jsonObjs.meshes;
    auto& arr = json::getArray(meshes);
    int i = 0;
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);
 
        auto pPrimitives = json::searchObject(obj, "primitives");
        if (!pPrimitives) LOG_FATAL("'primitives' field is required\n");
 
        adt::Array<Primitive> aPrimitives(this->pAlloc);
        auto pName = json::searchObject(obj, "name");
        auto name = pName ? json::getStringView(pName) : "";
 
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
                    .NORMAL = pNORMAL ? static_cast<decltype(Primitive::attributes.NORMAL)>(json::getLong(pNORMAL)) : NPOS,
                    .POSITION = pPOSITION ? static_cast<decltype(Primitive::attributes.POSITION)>(json::getLong(pPOSITION)) : NPOS,
                    .TEXCOORD_0 = pTEXCOORD_0 ? static_cast<decltype(Primitive::attributes.TEXCOORD_0)>(json::getLong(pTEXCOORD_0)) : NPOS,
                    .TANGENT = pTANGENT ? static_cast<decltype(Primitive::attributes.TANGENT)>(json::getLong(pTANGENT)) : NPOS,
                },
                .indices = pIndices ? static_cast<decltype(Primitive::indices)>(json::getLong(pIndices)) : NPOS,
                .material = pMaterial ? static_cast<decltype(Primitive::material)>(json::getLong(pMaterial)) : NPOS,
                .mode = pMode ? static_cast<decltype(Primitive::mode)>(json::getLong(pMode)) : PRIMITIVES::TRIANGLES,
            });
        }
 
        this->aMeshes.push({.aPrimitives = aPrimitives, .svName = name});
    }

#ifdef GLTF
    // LOG(OK, "meshes:\n");
    // for (auto& m : this->aMeshes)
    // {
    //     CERR("\tname: '{}'\n", m.svName);
    //     for (auto& p : m.aPrimitives)
    //     {
    //         CERR("\tattributes:\n");
    //         CERR("\t\tNORMAL: '{}', POSITION: '{}', TEXCOORD_0: '{}', TANGENT: '{}'\n",
    //              p.attributes.NORMAL, p.attributes.POSITION, p.attributes.TEXCOORD_0, p.attributes.TANGENT);
    //         CERR("\tindices: '{}', material: '{}, mode: '{}''\n\n", p.indices, p.material, getPrimitiveModeString(p.mode));
    //     }
    // }
#endif
}

void
Asset::processTexures()
{
    auto textures = this->jsonObjs.textures;
    if (!textures) return;

    auto& arr = json::getArray(textures);
    for (auto& tex : arr)
    {
        auto& obj = json::getObject(&tex);

        auto pSource = json::searchObject(obj, "source");
        auto pSampler = json::searchObject(obj, "sampler");

        this->aTextures.push({
            .source = pSource ? json::getLong(pSource) : NPOS,
            .sampler = pSampler ? json::getLong(pSampler) : NPOS
        });
    }
}

void
Asset::processMaterials()
{
    auto materials = this->jsonObjs.materials;
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

        this->aMaterials.push({
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
    auto imgs = this->jsonObjs.images;
    if (!imgs) return;

    auto& arr = json::getArray(imgs);
    for (auto& img : arr)
    {
        auto& obj = json::getObject(&img);

        auto pUri = json::searchObject(obj, "uri");
        if (pUri)
            this->aImages.push({json::getStringView(pUri)});
    }
}

void
Asset::processNodes()
{
    auto nodes = this->jsonObjs.nodes;
    auto& arr = json::getArray(nodes);
    for (auto& node : arr)
    {
        auto& obj = json::getObject(&node);

        Node nNode(this->pAlloc);

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

        this->aNodes.push(nNode);
    }

#ifdef GLTF
    // LOG(OK, "nodes:\n");
    // for (auto& node : this->aNodes)
    // {
    //     CERR("\tcamera: '{}'\n", node.camera);
    //     CERR("\tchildren: ");
    //     for (auto& c : node.children)
    //         CERR("{}, ", c);
    //     CERR("\n");

    //     union Type* ut = reinterpret_cast<union Type*>(&node.matrix);
    //     CERR("\tmatrix:\n{}\n", getUnionTypeString(ACCESSOR_TYPE::MAT4, *ut, "\t"));
    //     CERR("\tmesh: '{}'\n", node.mesh);
    //     ut = reinterpret_cast<union Type*>(&node.rotation);
    //     CERR("\trotation:\n{}\n", getUnionTypeString(ACCESSOR_TYPE::VEC4, *ut, "\t"));
    //     ut = reinterpret_cast<union Type*>(&node.translation);
    //     CERR("\ttranslation:\n{}\n", getUnionTypeString(ACCESSOR_TYPE::VEC3, *ut, "\t"));
    //     ut = reinterpret_cast<union Type*>(&node.scale);
    //     CERR("\tscale:\n{}\n", getUnionTypeString(ACCESSOR_TYPE::VEC3, *ut, "\t"));
    // }
#endif
}

} /* namespace gltf */
