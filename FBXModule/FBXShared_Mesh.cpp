/**
 * @file FBXUtilites_Mesh.cpp
 * @date 2019/04/12
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#include "stdafx.h"

#include <algorithm>

#include "FBXUtilites.h"
#include "FBXMath.h"
#include "FBXShared.h"


class _PositionSkin{
public:
    _PositionSkin()
        :
        skinInfos(nullptr)
    {}
    _PositionSkin(const Float3& _position, const FBXDynamicArray<FBXSkinElement>* _skinInfos)
        :
        position(_position),
        skinInfos(_skinInfos)
    {}
    _PositionSkin(const _PositionSkin& rhs)
        :
        position(rhs.position),
        skinInfos(rhs.skinInfos)
    {}


public:
    _PositionSkin& operator=(const _PositionSkin& rhs)noexcept{
        position = rhs.position;
        skinInfos = rhs.skinInfos;
        return (*this);
    }
    _PositionSkin& operator=(_PositionSkin&& rhs)noexcept{
        position = rhs.position;
        skinInfos = rhs.skinInfos;
        return (*this);
    }


public:
    inline size_t makeHash()const{
        size_t c, result = 2166136261U; //FNV1 hash

#define __CAL_RESULT result = (result * 16777619) ^ c;

        c = MakeHash(position.raw);
        __CAL_RESULT;

        if(skinInfos){
            for(const auto* pSkinInfo = skinInfos->Values; FBX_PTRDIFFU(pSkinInfo - skinInfos->Values) < skinInfos->Length; ++pSkinInfo){
                c = reinterpret_cast<size_t>(pSkinInfo->BindNode);
                __CAL_RESULT;

                c = MakeHash<1>(reinterpret_cast<const unsigned long*>(&pSkinInfo->Weight));
                __CAL_RESULT;
            }
        }

#undef __CAL_RESULT

        return result;
    }


public:
    Float3 position;
    const FBXDynamicArray<FBXSkinElement>* skinInfos;
};
static inline bool operator==(const _PositionSkin& lhs, const _PositionSkin& rhs){
    if(lhs.position != rhs.position)
        return false;


    if(lhs.skinInfos && (!rhs.skinInfos))
        return false;
    if((!lhs.skinInfos) && rhs.skinInfos)
        return false;
    if(lhs.skinInfos && rhs.skinInfos){
        if(lhs.skinInfos->Length != rhs.skinInfos->Length)
            return false;
        for(size_t idx = 0; idx < lhs.skinInfos->Length; ++idx){
            const auto& lhsInfo = lhs.skinInfos->Values[idx];
            const auto& rhsInfo = rhs.skinInfos->Values[idx];

            if(lhsInfo.BindNode != rhsInfo.BindNode)
                return false;
            if(lhsInfo.Weight != rhsInfo.Weight)
                return false;
        }
    }

    return true;
}


static fbx_unordered_map<unsigned int, unsigned int> ins_materialFinder;
static inline bool ins_addMaterial(UintContainer& matTable, unsigned int sceneMaterialIndex, unsigned int* iPoly){
    auto f = ins_materialFinder.find(sceneMaterialIndex);
    if(f == ins_materialFinder.end()){
        (*iPoly) = (unsigned int)matTable.size();
        matTable.emplace_back(sceneMaterialIndex);

        ins_materialFinder.emplace(sceneMaterialIndex, (*iPoly));
    }
    else
        (*iPoly) = f->second;

    return true;
}


bool SHRLoadMeshFromNode(MaterialTable& materialTable, ControlPointRemap& controlPointRemap, FbxNode* kNode, NodeData* pNodeData){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRLoadMeshFromNode(MaterialTable&, ControlPointRemap&, FbxNode*, NodeData*)");


    const fbx_string strName = ConvertString<FBX_CHAR>(kNode->GetName());

    const auto kMatGeometry = GetGeometry(kNode);
    auto* kMesh = (FbxMesh*)kNode->GetNodeAttribute();

    const auto polyCount = kMesh->GetPolygonCount();

    const auto ctrlPointsCount = kMesh->GetControlPointsCount();
    auto* ctrlPoints = kMesh->GetControlPoints();

    const auto layerCount = kMesh->GetLayerCount();

    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
        auto beginIndex = kMesh->GetPolygonVertexIndex(iPoly);
        if(beginIndex == -1)
            continue;

        auto* vertices = &kMesh->GetPolygonVertices()[beginIndex];
        auto vertCount = kMesh->GetPolygonSize(iPoly);

        for(auto iVert = decltype(vertCount){ 0 }; iVert < vertCount; ++iVert){
            auto vertID = vertices[iVert];
            auto ctrlPointIndex = kMesh->GetPolygonVertex(iPoly, iVert);

            if(vertID != ctrlPointIndex){
                fbx_string msg = FBX_TEXT("vertex index and control point index are not matched");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }
        }
    }

    controlPointRemap.clear();
    controlPointRemap.resize((size_t)ctrlPointsCount);

    int t = 0;
    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
        auto polySize = kMesh->GetPolygonSize(iPoly);

        if(polySize != 3){
            fbx_string msg = FBX_TEXT("polygon size must be 3");
            msg += FBX_TEXT("(errored in \"");
            msg += strName;
            msg += FBX_TEXT("\")");
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return false;
        }

        for(auto iVert = decltype(polySize){ 0 }; iVert < 3; ++iVert){
            auto ctrlPointIndex = kMesh->GetPolygonVertex(iPoly, iVert);

            if(ctrlPointIndex < 0){
                fbx_string msg = FBX_TEXT("vertex index must be bigger than or equal to 0");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            auto flatIndex = iPoly * 3 + iVert;
            if(t != flatIndex){
                fbx_string msg = FBX_TEXT("vertex index is invalid");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            controlPointRemap[ctrlPointIndex].emplace((unsigned int)flatIndex);

            ++t;
        }
    }

    ins_materialFinder.clear();

    { // load mesh
        auto& materials = pNodeData->bufMaterials;

        auto& positions = pNodeData->bufPositions;
        auto& indices = pNodeData->bufIndices;

        auto& layers = pNodeData->bufLayers;

        { // reserve
            materials.clear();
            materials.reserve((size_t)kNode->GetMaterialCount());

            positions.resize(size_t(polyCount) * 3u);
            indices.resize((size_t)polyCount);

            layers.resize((size_t)layerCount);
        }

        {
            unsigned int cnt = 0u;
            for(auto& i : indices){
                for(auto& k : i.raw)
                    k = cnt++;
            }
        }

        for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
            for(int iVert = 0; iVert < 3; ++iVert){
                auto ctrlPointIndex = kMesh->GetPolygonVertex(iPoly, iVert);
                auto flatIndex = iPoly * 3 + iVert;

                { // position
                    auto& vert = positions[flatIndex];
                    auto& pos = ctrlPoints[ctrlPointIndex];

                    CopyArrayData(vert.mData, pos.mData);

                    vert = Transform44(kMatGeometry, vert);
                }
            }
        }

        for(auto iLayer = decltype(layerCount){ 0 }; iLayer < layerCount; ++iLayer){
            auto kLayer = kMesh->GetLayer(iLayer);
            if(!kLayer)
                continue;

            const auto* kMaterials = kLayer->GetMaterials();

            const auto* kVertColors = kLayer->GetVertexColors();
            const auto* kTexUVs = kLayer->GetUVs();
            const auto* kNormals = kLayer->GetNormals();
            const auto* kBinormals = kLayer->GetBinormals();
            const auto* kTangents = kLayer->GetTangents();

            auto& pLayer = layers[iLayer];

            if(kMaterials){
                auto& pObject = pLayer.materials;
                auto* kObject = kMaterials;

                pObject.resize((size_t)polyCount, (unsigned int)(-1));

                switch(kObject->GetMappingMode()){
                case FbxLayerElement::eByPolygon:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        auto& poly = pObject[iPoly];

                        auto kIdxMaterial = kObject->GetIndexArray().GetAt(iPoly);
                        auto* kSurfMaterial = kNode->GetMaterial(kIdxMaterial);
                        auto kIdxSceneMaterial = materialTable.emplace(kSurfMaterial);

                        if(!ins_addMaterial(materials, kIdxSceneMaterial, &poly)){
                            fbx_string msg = FBX_TEXT("cannot load material \"");
                            msg += ConvertString<FBX_CHAR>(kSurfMaterial->GetName());
                            msg += FBX_TEXT("\" while reading material by FbxLayerElement::eByPolygon");
                            msg += FBX_TEXT("(errored in \"");
                            msg += strName;
                            msg += FBX_TEXT("\")");
                            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                            return false;
                        }
                    }
                    break;

                case FbxLayerElement::eAllSame:
                {
                    unsigned int matIndex = (unsigned int)(-1);
                    {
                        auto kIdxMaterial = kObject->GetIndexArray().GetAt(0);
                        auto* kSurfMaterial = kNode->GetMaterial(kIdxMaterial);
                        auto kIdxSceneMaterial = materialTable.emplace(kSurfMaterial);

                        if(!ins_addMaterial(materials, kIdxSceneMaterial, &matIndex)){
                            fbx_string msg = FBX_TEXT("cannot load material \"");
                            msg += ConvertString<FBX_CHAR>(kSurfMaterial->GetName());
                            msg += FBX_TEXT("\" while reading material by FbxLayerElement::eAllSame");
                            msg += FBX_TEXT("(errored in \"");
                            msg += strName;
                            msg += FBX_TEXT("\")");
                            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                            return false;
                        }
                    }

                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        auto& poly = pObject[iPoly];

                        poly = matIndex;
                    }

                    break;
                }

                default:
                {
                    fbx_string msg = FBX_TEXT("material has unsupported mapping mode");
                    msg += FBX_TEXT("(errored in \"");
                    msg += strName;
                    msg += FBX_TEXT("\")");
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                    return false;
                }
                }
            }

            if(kVertColors){
                auto& pObject = pLayer.colors;
                auto* kObject = kVertColors;

                pObject.resize(positions.size(), FbxDouble4(1, 1, 1, 1));

                switch(kObject->GetMappingMode()){
                case FbxLayerElement::eByControlPoint:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        for(int iVert = 0; iVert < 3; ++iVert){
                            auto ctrlPointIndex = kMesh->GetPolygonVertex(iPoly, iVert);
                            auto flatIndex = iPoly * 3 + iVert;

                            auto& vert = pObject[flatIndex];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxColor, int>(vert.mData, kObject->GetDirectArray().GetAt(ctrlPointIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxColor, int>(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(ctrlPointIndex)));
                                break;

                            default:
                            {
                                fbx_string msg = FBX_TEXT("vertex color has unsupported reference mode");
                                msg += FBX_TEXT("(errored in \"");
                                msg += strName;
                                msg += FBX_TEXT("\")");
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }
                        }
                    }
                    break;
                
                case FbxLayerElement::eByPolygonVertex:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        for(int iVert = 0; iVert < 3; ++iVert){
                            auto flatIndex = iPoly * 3 + iVert;

                            auto& vert = pObject[flatIndex];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxColor, int>(vert.mData, kObject->GetDirectArray().GetAt(flatIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxColor, int>(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(flatIndex)));
                                break;

                            default:
                            {
                                fbx_string msg = FBX_TEXT("vertex color has unsupported reference mode");
                                msg += FBX_TEXT("(errored in \"");
                                msg += strName;
                                msg += FBX_TEXT("\")");
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }
                        }
                    }
                    break;

                default:
                {
                    fbx_string msg = FBX_TEXT("vertex color has unsupported mapping mode");
                    msg += FBX_TEXT("(errored in \"");
                    msg += strName;
                    msg += FBX_TEXT("\")");
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                    return false;
                }
                }
            }

            if(kTexUVs){
                auto& pObject = pLayer.texcoords;
                auto* kObject = kTexUVs;

                {
                    pObject.name = kObject->GetName();
                    pObject.table.resize(positions.size(), FbxDouble2(0, 0));
                }

                switch(kObject->GetMappingMode()){
                case FbxLayerElement::eByControlPoint:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        for(int iVert = 0; iVert < 3; ++iVert){
                            auto ctrlPointIndex = kMesh->GetPolygonVertex(iPoly, iVert);
                            auto flatIndex = iPoly * 3 + iVert;

                            auto& vert = pObject.table[flatIndex];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector2, int>(vert.mData, kObject->GetDirectArray().GetAt(ctrlPointIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector2, int>(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(ctrlPointIndex)));
                                break;

                            default:
                            {
                                fbx_string msg = FBX_TEXT("vertex color has unsupported reference mode");
                                msg += FBX_TEXT("(errored in \"");
                                msg += strName;
                                msg += FBX_TEXT("\")");
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }
                        }
                    }
                    break;

                case FbxLayerElement::eByPolygonVertex:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        for(int iVert = 0; iVert < 3; ++iVert){
                            auto uvIndex = kMesh->GetTextureUVIndex(iPoly, iVert);
                            auto flatIndex = iPoly * 3 + iVert;

                            auto& vert = pObject.table[flatIndex];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector2, int>(vert.mData, kObject->GetDirectArray().GetAt(uvIndex));
                                break;

                            default:
                            {
                                fbx_string msg = FBX_TEXT("vertex color has unsupported reference mode");
                                msg += FBX_TEXT("(errored in \"");
                                msg += strName;
                                msg += FBX_TEXT("\")");
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }
                        }
                    }
                    break;

                default:
                {
                    fbx_string msg = FBX_TEXT("texcoord has unsupported mapping mode");
                    msg += FBX_TEXT("(errored in \"");
                    msg += strName;
                    msg += FBX_TEXT("\")");
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                    return false;
                }
                }

                for(auto& val : pObject.table){
                    auto& y = val[1];
                    y = 1. - y;
                }
            }

            if(kNormals){
                auto& pObject = pLayer.normals;
                auto* kObject = kNormals;

                pObject.resize(positions.size(), FbxDouble3(0, 0, 0));

                switch(kObject->GetMappingMode()){
                case FbxLayerElement::eByControlPoint:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        for(int iVert = 0; iVert < 3; ++iVert){
                            auto ctrlPointIndex = kMesh->GetPolygonVertex(iPoly, iVert);
                            auto flatIndex = iPoly * 3 + iVert;

                            auto& vert = pObject[flatIndex];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector4, int>(vert.mData, kObject->GetDirectArray().GetAt(ctrlPointIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector4, int>(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(ctrlPointIndex)));
                                break;

                            default:
                            {
                                fbx_string msg = FBX_TEXT("vertex normal has unsupported reference mode");
                                msg += FBX_TEXT("(errored in \"");
                                msg += strName;
                                msg += FBX_TEXT("\")");
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }

                            vert = Normalize3(vert);
                        }
                    }
                    break;

                case FbxLayerElement::eByPolygonVertex:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        for(int iVert = 0; iVert < 3; ++iVert){
                            auto flatIndex = iPoly * 3 + iVert;

                            auto& vert = pObject[flatIndex];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector4, int>(vert.mData, kObject->GetDirectArray().GetAt(flatIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector4, int>(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(flatIndex)));
                                break;

                            default:
                            {
                                fbx_string msg = FBX_TEXT("vertex normal has unsupported reference mode");
                                msg += FBX_TEXT("(errored in \"");
                                msg += strName;
                                msg += FBX_TEXT("\")");
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }

                            vert = Normalize3(vert);
                        }
                    }
                    break;

                default:
                {
                    fbx_string msg = FBX_TEXT("vertex normal has unsupported mapping mode");
                    msg += FBX_TEXT("(errored in \"");
                    msg += strName;
                    msg += FBX_TEXT("\")");
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                    return false;
                }
                }
            }

            if(kBinormals){
                auto& pObject = pLayer.binormals;
                auto* kObject = kBinormals;

                pObject.resize(positions.size(), FbxDouble3(0, 0, 0));

                switch(kObject->GetMappingMode()){
                case FbxLayerElement::eByControlPoint:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        for(int iVert = 0; iVert < 3; ++iVert){
                            auto ctrlPointIndex = kMesh->GetPolygonVertex(iPoly, iVert);
                            auto flatIndex = iPoly * 3 + iVert;

                            auto& vert = pObject[flatIndex];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector4, int>(vert.mData, kObject->GetDirectArray().GetAt(ctrlPointIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector4, int>(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(ctrlPointIndex)));
                                break;

                            default:
                            {
                                fbx_string msg = FBX_TEXT("vertex binormal has unsupported reference mode");
                                msg += FBX_TEXT("(errored in \"");
                                msg += strName;
                                msg += FBX_TEXT("\")");
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }

                            vert = Normalize3(vert);
                        }
                    }
                    break;

                case FbxLayerElement::eByPolygonVertex:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        for(int iVert = 0; iVert < 3; ++iVert){
                            auto flatIndex = iPoly * 3 + iVert;

                            auto& vert = pObject[flatIndex];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector4, int>(vert.mData, kObject->GetDirectArray().GetAt(flatIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector4, int>(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(flatIndex)));
                                break;

                            default:
                            {
                                fbx_string msg = FBX_TEXT("vertex binormal has unsupported reference mode");
                                msg += FBX_TEXT("(errored in \"");
                                msg += strName;
                                msg += FBX_TEXT("\")");
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }

                            vert = Normalize3(vert);
                        }
                    }
break;

                default:
                {
                    fbx_string msg = FBX_TEXT("vertex binormal has unsupported mapping mode");
                    msg += FBX_TEXT("(errored in \"");
                    msg += strName;
                    msg += FBX_TEXT("\")");
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                    return false;
                }
                }
            }

            if(kTangents){
                auto& pObject = pLayer.tangents;
                auto* kObject = kTangents;

                pObject.resize(positions.size(), FbxDouble3(0, 0, 0));

                switch(kObject->GetMappingMode()){
                case FbxLayerElement::eByControlPoint:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        for(int iVert = 0; iVert < 3; ++iVert){
                            auto ctrlPointIndex = kMesh->GetPolygonVertex(iPoly, iVert);
                            auto flatIndex = iPoly * 3 + iVert;

                            auto& vert = pObject[flatIndex];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector4, int>(vert.mData, kObject->GetDirectArray().GetAt(ctrlPointIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector4, int>(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(ctrlPointIndex)));
                                break;

                            default:
                            {
                                fbx_string msg = FBX_TEXT("vertex tangent has unsupported reference mode");
                                msg += FBX_TEXT("(errored in \"");
                                msg += strName;
                                msg += FBX_TEXT("\")");
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }

                            vert = Normalize3(vert);
                        }
                    }
                    break;

                case FbxLayerElement::eByPolygonVertex:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        for(int iVert = 0; iVert < 3; ++iVert){
                            auto flatIndex = iPoly * 3 + iVert;

                            auto& vert = pObject[flatIndex];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector4, int>(vert.mData, kObject->GetDirectArray().GetAt(flatIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData<_countof(vert.mData), FbxDouble, FbxVector4, int>(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(flatIndex)));
                                break;

                            default:
                            {
                                fbx_string msg = FBX_TEXT("vertex tangent has unsupported reference mode");
                                msg += FBX_TEXT("(errored in \"");
                                msg += strName;
                                msg += FBX_TEXT("\")");
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }

                            vert = Normalize3(vert);
                        }
                    }
                    break;

                default:
                {
                    fbx_string msg = FBX_TEXT("vertex tangent has unsupported mapping mode");
                    msg += FBX_TEXT("(errored in \"");
                    msg += strName;
                    msg += FBX_TEXT("\")");
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                    return false;
                }
                }
            }
        }
    }

    {
        auto& materials = pNodeData->bufMaterials;
        if(materials.size() > 1u){
            auto& layers = pNodeData->bufLayers;

            decltype(pNodeData->bufMaterials) oldMaterials(materials);
            std::sort(materials.begin(), materials.end());

            if(materials != oldMaterials){
                decltype(pNodeData->bufMaterials) idxConverter(materials.size());
                {
                    fbx_unordered_map<unsigned int, unsigned int, CustomHasher<unsigned int>> matIndexer;
                    {
                        matIndexer.rehash(materials.size());

                        for(size_t edxMat = materials.size(), idxMat = 0u; idxMat < edxMat; ++idxMat)
                            matIndexer[materials[idxMat]] = idxMat;
                    }

                    for(size_t edxMat = idxConverter.size(), idxMat = 0u; idxMat < edxMat; ++idxMat)
                        idxConverter[idxMat] = matIndexer[oldMaterials[idxMat]];
                }

                for(auto& iLayer : layers){
                    for(auto& iMat : iLayer.materials)
                        iMat = idxConverter[iMat];
                }
            }
        }
    }

    return true;
}

bool SHRInitMeshNode(FbxManager* kSDKManager, FbxScene* kScene, ControlPointMergeMap& ctrlPointMergeMap, const FBXMesh* pNode, FbxNode* kNode){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRInitMeshNode(FbxManager*, FbxScene*, ControlPointMergeMap&, const FBXMesh*, FbxNode*)");


    const FBXSkinnedMesh* pSkinnedNode = nullptr;
    if(FBXTypeHasMember(pNode->getID(), FBXType::FBXType_SkinnedMesh))
        pSkinnedNode = static_cast<decltype(pSkinnedNode)>(pNode);

    const fbx_string strMeshName = pNode->Name.Values;
    auto* kMesh = kNode->GetMesh();

    {
        for(auto* pMaterial = pNode->Materials.Values; FBX_PTRDIFFU(pMaterial - pNode->Materials.Values) < pNode->Materials.Length; ++pMaterial){
            auto* kMaterial = kScene->GetMaterial((int)*pMaterial);
            if(!kMaterial){
                fbx_string msg = FBX_TEXT("cannot find material index ");
                msg += ToString<FBX_CHAR>(*pMaterial);
                msg += FBX_TEXT(" on scene materials");
                msg += FBX_TEXT("(errored in \"");
                msg += strMeshName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            const fbx_string strMaterialName = ConvertString<FBX_CHAR>(kMaterial->GetName());

            if(kNode->AddMaterial(kMaterial) < 0){
                fbx_string msg = FBX_TEXT("failed to add material \"");
                msg += strMaterialName;
                msg += FBX_TEXT("\"");
                msg += FBX_TEXT("(errored in \"");
                msg += strMeshName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }
        }
    }

    { // create control point & make convert table
        OverlapReducer<_PositionSkin> reducer(pNode->Vertices.Length, [&pNode, &pSkinnedNode](FBX_SIZE idx)->_PositionSkin{
            const auto& oldVal = pNode->Vertices.Values[idx];

            Float3 newVal;
            CopyArrayData(newVal.raw, oldVal.Values);

            const FBXDynamicArray<FBXSkinElement>* skinInfos = nullptr;
            if(pSkinnedNode)
                skinInfos = &pSkinnedNode->SkinInfos.Values[idx];

            return _PositionSkin(newVal, skinInfos);
        });

        reducer.build([](const _PositionSkin& v)->size_t{ return v.makeHash(); });

        const auto& convTable = reducer.getConvertedTable();

        kMesh->SetControlPointCount((int)convTable.size());

        auto* kPoints = kMesh->GetControlPoints();
        for(auto itPos = convTable.cbegin(), etPos = convTable.cend(); itPos != etPos; ++itPos, ++kPoints){
            CopyArrayData(kPoints->mData, itPos->position.raw);
            kPoints->mData[3] = 1.;
        }

        ctrlPointMergeMap = std::move(reducer.getOldToConvertIndexer());
    }

    { // create polygon
        for(auto* pPoly = pNode->Indices.Values; FBX_PTRDIFFU(pPoly - pNode->Indices.Values) < pNode->Indices.Length; ++pPoly){
            kMesh->BeginPolygon();

            for(const auto& iPolyIndex : pPoly->Values){
                const auto convIndex = ctrlPointMergeMap[iPolyIndex];

                kMesh->AddPolygon((int)convIndex);
            }

            kMesh->EndPolygon();
        }
    }

    // reserve layer
    for(size_t idxLayer = 0u; idxLayer < pNode->LayeredElements.Length; ++idxLayer){
        auto* kLayer = kMesh->GetLayer((int)idxLayer);
        if(!kLayer){
            const auto idxNewLayer = kMesh->CreateLayer();
            if(idxNewLayer < 0){
                fbx_string msg = FBX_TEXT("failed to create layer");
                msg += FBX_TEXT("(errored in \"");
                msg += strMeshName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }
            else if(idxLayer != idxNewLayer){
                fbx_string msg = FBX_TEXT("the created layer has unexpected index number");
                msg += FBX_TEXT("(errored in \"");
                msg += strMeshName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            kLayer = kMesh->GetLayer(idxNewLayer);
        }
    }

    for(size_t idxLayer = 0u; idxLayer < pNode->LayeredElements.Length; ++idxLayer){
        const auto& iLayer = pNode->LayeredElements.Values[idxLayer];
        auto* kLayer = kMesh->GetLayer((int)idxLayer);

        if(iLayer.Material.Length){
            auto* kMaterial = FbxLayerElementMaterial::Create(kMesh, "");
            if(!kMaterial){
                fbx_string msg = FBX_TEXT("failed to create element material in the layer ") + ToString<FBX_CHAR>(idxLayer);
                msg += FBX_TEXT("(errored in \"");
                msg += strMeshName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            bool hasSameValue = true;
            const auto matValue = iLayer.Material.Values[0];
            if(iLayer.Material.Length > 1u){
                for(auto* pMat = iLayer.Material.Values + 1; FBX_PTRDIFFU(pMat - iLayer.Material.Values) < iLayer.Material.Length; ++pMat){
                    if(matValue != (*pMat)){
                        hasSameValue = false;
                        break;
                    }
                }
            }

            if(hasSameValue){
                kMaterial->SetMappingMode(FbxGeometryElement::eAllSame);
                kMaterial->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

                auto& kIndices = kMaterial->GetIndexArray();
                kIndices.Add(matValue);
            }
            else{
                kMaterial->SetMappingMode(FbxGeometryElement::eByPolygon);
                kMaterial->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

                auto& kIndices = kMaterial->GetIndexArray();
                for(size_t idxAttribute = 0u; idxAttribute < pNode->Attributes.Length; ++idxAttribute){
                    const auto& iAttribute = pNode->Attributes.Values[idxAttribute];
                    const auto iMaterial = iLayer.Material.Values[idxAttribute];

                    for(auto idxPoly = iAttribute.IndexStart, edxPoly = iAttribute.IndexStart + iAttribute.IndexCount; idxPoly < edxPoly; ++idxPoly)
                        kIndices.Add(iMaterial);
                }
            }

            kLayer->SetMaterials(kMaterial);
        }

        if(iLayer.Color.Length){
            auto* kColor = FbxLayerElementVertexColor::Create(kMesh, "");
            if(!kColor){
                fbx_string msg = FBX_TEXT("failed to create element vertex color in the layer ") + ToString<FBX_CHAR>(idxLayer);
                msg += FBX_TEXT("(errored in \"");
                msg += strMeshName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            kColor->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
            kColor->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

            OverlapReducer<Float4> reducer(iLayer.Color.Length, [&iLayer](FBX_SIZE idx)->Float4{
                const auto& oldVal = iLayer.Color.Values[idx];

                Float4 newVal;
                CopyArrayData(newVal.raw, oldVal.Values);

                return newVal;
            });

            reducer.build([](const Float4& v)->size_t{ return MakeHash<_countof(v.raw)>(reinterpret_cast<const unsigned long*>(v.raw)); });

            auto& kDirect = kColor->GetDirectArray();
            for(const auto& iVal : reducer.getConvertedTable()){
                FbxColor kVal(iVal.x, iVal.y, iVal.z, iVal.w);
                kDirect.Add(kVal);
            }

            auto& kIndices = kColor->GetIndexArray();
            for(auto* pPoly = pNode->Indices.Values; FBX_PTRDIFFU(pPoly - pNode->Indices.Values) < pNode->Indices.Length; ++pPoly){
                for(const auto& idxPolyIndex : pPoly->Values)
                    kIndices.Add((int)reducer.getOldToConvertIndexer()[idxPolyIndex]);
            }

            kLayer->SetVertexColors(kColor);
        }

        if(iLayer.Normal.Length){
            auto* kNormal = FbxLayerElementNormal::Create(kMesh, "");
            if(!kNormal){
                fbx_string msg = FBX_TEXT("failed to create element normal in the layer ") + ToString<FBX_CHAR>(idxLayer);
                msg += FBX_TEXT("(errored in \"");
                msg += strMeshName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            kNormal->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
            kNormal->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

            OverlapReducer<Float3> reducer(iLayer.Normal.Length, [&iLayer](FBX_SIZE idx)->Float3{
                const auto& oldVal = iLayer.Normal.Values[idx];

                Float3 newVal;
                CopyArrayData(newVal.raw, oldVal.Values);
                newVal = Normalize3(newVal);

                return newVal;
            });

            reducer.build([](const Float3& v)->size_t{ return MakeHash<_countof(v.raw)>(reinterpret_cast<const unsigned long*>(v.raw)); });

            auto& kDirect = kNormal->GetDirectArray();
            for(const auto& iVal : reducer.getConvertedTable()){
                FbxVector4 kVal(iVal.x, iVal.y, iVal.z);
                kDirect.Add(kVal);
            }

            auto& kIndices = kNormal->GetIndexArray();
            for(auto* pPoly = pNode->Indices.Values; FBX_PTRDIFFU(pPoly - pNode->Indices.Values) < pNode->Indices.Length; ++pPoly){
                for(const auto& idxPolyIndex : pPoly->Values)
                    kIndices.Add((int)reducer.getOldToConvertIndexer()[idxPolyIndex]);
            }

            kLayer->SetNormals(kNormal);
        }

        if(iLayer.Binormal.Length){
            auto* kBinormal = FbxLayerElementBinormal::Create(kMesh, "");
            if(!kBinormal){
                fbx_string msg = FBX_TEXT("failed to create element binormal in the layer ") + ToString<FBX_CHAR>(idxLayer);
                msg += FBX_TEXT("(errored in \"");
                msg += strMeshName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            kBinormal->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
            kBinormal->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

            OverlapReducer<Float3> reducer(iLayer.Binormal.Length, [&iLayer](FBX_SIZE idx)->Float3{
                const auto& oldVal = iLayer.Binormal.Values[idx];

                Float3 newVal;
                CopyArrayData(newVal.raw, oldVal.Values);
                newVal = Normalize3(newVal);

                return newVal;
            });

            reducer.build([](const Float3& v)->size_t{ return MakeHash<_countof(v.raw)>(reinterpret_cast<const unsigned long*>(v.raw)); });

            auto& kDirect = kBinormal->GetDirectArray();
            for(const auto& iVal : reducer.getConvertedTable()){
                FbxVector4 kVal(iVal.x, iVal.y, iVal.z);
                kDirect.Add(kVal);
            }

            auto& kIndices = kBinormal->GetIndexArray();
            for(auto* pPoly = pNode->Indices.Values; FBX_PTRDIFFU(pPoly - pNode->Indices.Values) < pNode->Indices.Length; ++pPoly){
                for(const auto& idxPolyIndex : pPoly->Values)
                    kIndices.Add((int)reducer.getOldToConvertIndexer()[idxPolyIndex]);
            }

            kLayer->SetBinormals(kBinormal);
        }

        if(iLayer.Tangent.Length){
            auto* kTangent = FbxLayerElementTangent::Create(kMesh, "");
            if(!kTangent){
                fbx_string msg = FBX_TEXT("failed to create element tangent in the layer ") + ToString<FBX_CHAR>(idxLayer);
                msg += FBX_TEXT("(errored in \"");
                msg += strMeshName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            kTangent->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
            kTangent->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

            OverlapReducer<Float3> reducer(iLayer.Tangent.Length, [&iLayer](FBX_SIZE idx)->Float3{
                const auto& oldVal = iLayer.Tangent.Values[idx];

                Float3 newVal;
                CopyArrayData(newVal.raw, oldVal.Values);
                newVal = Normalize3(newVal);

                return newVal;
            });

            reducer.build([](const Float3& v)->size_t{ return MakeHash<_countof(v.raw)>(reinterpret_cast<const unsigned long*>(v.raw)); });

            auto& kDirect = kTangent->GetDirectArray();
            for(const auto& iVal : reducer.getConvertedTable()){
                FbxVector4 kVal(iVal.x, iVal.y, iVal.z);
                kDirect.Add(kVal);
            }

            auto& kIndices = kTangent->GetIndexArray();
            for(auto* pPoly = pNode->Indices.Values; FBX_PTRDIFFU(pPoly - pNode->Indices.Values) < pNode->Indices.Length; ++pPoly){
                for(const auto& idxPolyIndex : pPoly->Values)
                    kIndices.Add((int)reducer.getOldToConvertIndexer()[idxPolyIndex]);
            }

            kLayer->SetTangents(kTangent);
        }

        if(iLayer.Texcoord.Length){
            auto* kUV = FbxLayerElementUV::Create(kMesh, "");
            if(!kUV){
                fbx_string msg = FBX_TEXT("failed to create element UV in the layer ") + ToString<FBX_CHAR>(idxLayer);
                msg += FBX_TEXT("(errored in \"");
                msg += strMeshName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            kUV->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
            kUV->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

            OverlapReducer<Float2> reducer(iLayer.Texcoord.Length, [&iLayer](FBX_SIZE idx)->Float2{
                const auto& oldVal = iLayer.Texcoord.Values[idx];

                Float2 newVal;
                CopyArrayData(newVal.raw, oldVal.Values);
                newVal.y = 1.f - newVal.y;

                return newVal;
            });

            reducer.build([](const Float2& v)->size_t{ return MakeHash<_countof(v.raw)>(reinterpret_cast<const unsigned long*>(v.raw)); });

            auto& kDirect = kUV->GetDirectArray();
            for(const auto& iVal : reducer.getConvertedTable()){
                FbxVector2 kVal(iVal.x, iVal.y);
                kDirect.Add(kVal);
            }

            auto& kIndices = kUV->GetIndexArray();
            for(auto* pPoly = pNode->Indices.Values; FBX_PTRDIFFU(pPoly - pNode->Indices.Values) < pNode->Indices.Length; ++pPoly){
                for(const auto& idxPolyIndex : pPoly->Values)
                    kIndices.Add((int)reducer.getOldToConvertIndexer()[idxPolyIndex]);
            }

            kLayer->SetUVs(kUV);
        }
    }

    return true;
}
