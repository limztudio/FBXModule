/**
* @file FBXUtilites_Mesh.cpp
* @date 2019/04/12
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

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
    _PositionSkin& operator=(const _PositionSkin& rhs){
        position = rhs.position;
        skinInfos = rhs.skinInfos;
        return (*this);
    }
    _PositionSkin& operator=(_PositionSkin&& rhs){
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


bool SHRLoadMeshFromNode(ControlPointRemap& controlPointRemap, FbxNode* kNode, NodeData* pNodeData){
    static const char __name_of_this_func[] = "SHRLoadMeshFromNode(ControlPointRemap&, FbxNode*, NodeData*)";


    const std::string strName = kNode->GetName();

    auto* kMesh = (FbxMesh*)kNode->GetNodeAttribute();

    auto edgeCount = kMesh->GetMeshEdgeCount();
    auto polyCount = kMesh->GetPolygonCount();

    auto ctrlPointsCount = kMesh->GetControlPointsCount();
    auto* ctrlPoints = kMesh->GetControlPoints();

    auto layerCount = kMesh->GetLayerCount();

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
                std::string msg = "vertex index and control point index are not matched";
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }
        }
    }

    controlPointRemap.clear();
    controlPointRemap.resize(ctrlPointsCount);

    std::vector<int> flatIndexToCtrlPoint(polyCount * 3);

    int t = 0;
    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
        auto polySize = kMesh->GetPolygonSize(iPoly);

        if(polySize != 3){
            std::string msg = "polygon size must be 3";
            msg += "(errored in \"";
            msg += strName;
            msg += "\")";
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return false;
        }

        for(auto iVert = decltype(polySize){ 0 }; iVert < 3; ++iVert){
            auto ctrlPointIndex = kMesh->GetPolygonVertex(iPoly, iVert);

            if(ctrlPointIndex < 0){
                std::string msg = "vertex index must be bigger than or equal to 0";
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            auto flatIndex = iPoly * 3 + iVert;
            if(t != flatIndex){
                std::string msg = "vertex index is invalid";
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            controlPointRemap[ctrlPointIndex].emplace(flatIndex);
            flatIndexToCtrlPoint[flatIndex] = ctrlPointIndex;

            ++t;
        }
    }

    { // load mesh
        auto& positions = pNodeData->bufPositions;
        auto& indices = pNodeData->bufIndices;

        auto& layers = pNodeData->bufLayers;

        { // reserve
            {
                positions.resize(polyCount * 3);
                indices.resize(polyCount);

                layers.resize((size_t)layerCount);
            }
        }

        {
            int cnt = 0;
            for(auto& i : indices){
                for(auto& k : i.raw)
                    k = cnt++;
            }
        }

        auto kRefGeometry = GetGeometry(kNode);

        for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
            for(int iVert = 0; iVert < 3; ++iVert){
                auto ctrlPointIndex = kMesh->GetPolygonVertex(iPoly, iVert);
                auto flatIndex = iPoly * 3 + iVert;

                { // position
                    auto& vert = positions[flatIndex];
                    auto& pos = ctrlPoints[ctrlPointIndex];

                    CopyArrayData(vert.mData, pos.mData);

                    vert = Transform44(kRefGeometry, vert);
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

                pObject.resize(polyCount, -1);

                switch(kObject->GetMappingMode()){
                case FbxLayerElement::eByPolygon:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        auto& poly = pObject[iPoly];

                        poly = kObject->GetIndexArray().GetAt(iPoly);
                    }
                    break;

                case FbxLayerElement::eAllSame:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        auto& poly = pObject[iPoly];

                        poly = kObject->GetIndexArray().GetAt(0);
                    }
                    break;

                default:
                {
                    std::string msg = "material has unsupported mapping mode";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
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
                                CopyArrayData(vert.mData, kObject->GetDirectArray().GetAt(ctrlPointIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(ctrlPointIndex)));
                                break;

                            default:
                            {
                                std::string msg = "vertex color has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
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
                                CopyArrayData(vert.mData, kObject->GetDirectArray().GetAt(flatIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(flatIndex)));
                                break;

                            default:
                            {
                                std::string msg = "vertex color has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }
                        }
                    }
                    break;

                default:
                {
                    std::string msg = "vertex color has unsupported mapping mode";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
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
                                CopyArrayData(vert.mData, kObject->GetDirectArray().GetAt(ctrlPointIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(ctrlPointIndex)));
                                break;

                            default:
                            {
                                std::string msg = "vertex color has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
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
                                CopyArrayData(vert.mData, kObject->GetDirectArray().GetAt(uvIndex));
                                break;

                            default:
                            {
                                std::string msg = "vertex color has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }

                            vert[1] = 1. - vert[1];
                        }
                    }
                    break;

                default:
                {
                    std::string msg = "texcoord has unsupported mapping mode";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
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
                case FbxLayerElement::eByPolygonVertex:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        for(int iVert = 0; iVert < 3; ++iVert){
                            auto flatIndex = iPoly * 3 + iVert;

                            auto& vert = pObject[flatIndex];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                                CopyArrayData(vert.mData, kObject->GetDirectArray().GetAt(flatIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(flatIndex)));
                                break;

                            default:
                            {
                                std::string msg = "vertex normal has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }

                            vert = Transform33(kRefGeometry, vert);
                            vert = Normalize3(vert);
                        }
                    }
                    break;

                default:
                {
                    std::string msg = "vertex normal has unsupported mapping mode";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
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
                case FbxLayerElement::eByPolygonVertex:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        for(int iVert = 0; iVert < 3; ++iVert){
                            auto flatIndex = iPoly * 3 + iVert;

                            auto& vert = pObject[flatIndex];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                                CopyArrayData(vert.mData, kObject->GetDirectArray().GetAt(flatIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(flatIndex)));
                                break;

                            default:
                            {
                                std::string msg = "vertex binormal has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }

                            vert = Transform33(kRefGeometry, vert);
                            vert = Normalize3(vert);
                        }
                    }
                    break;

                default:
                {
                    std::string msg = "vertex binormal has unsupported mapping mode";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
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
                case FbxLayerElement::eByPolygonVertex:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        for(int iVert = 0; iVert < 3; ++iVert){
                            auto flatIndex = iPoly * 3 + iVert;

                            auto& vert = pObject[flatIndex];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                                CopyArrayData(vert.mData, kObject->GetDirectArray().GetAt(flatIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                CopyArrayData(vert.mData, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(flatIndex)));
                                break;

                            default:
                            {
                                std::string msg = "vertex tangent has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
                                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                                return false;
                            }
                            }

                            vert = Transform33(kRefGeometry, vert);
                            vert = Normalize3(vert);
                        }
                    }
                    break;

                default:
                {
                    std::string msg = "vertex tangent has unsupported mapping mode";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                    return false;
                }
                }
            }
        }
    }

    return true;
}

bool SHRInitMeshNode(FbxManager* kSDKManager, ControlPointMergeMap& ctrlPointMergeMap, const FBXMesh* pNode, FbxNode* kNode){
    static const char __name_of_this_func[] = "SHRInitMeshNode(FbxManager*, ControlPointMergeMap&, const FBXMesh*, FbxNode*)";


    const FBXSkinnedMesh* pSkinnedNode = nullptr;
    if(FBXTypeHasMember(pNode->getID(), FBXType::FBXType_SkinnedMesh))
        pSkinnedNode = static_cast<decltype(pSkinnedNode)>(pNode);

    const std::string strName = pNode->Name.Values;
    auto* kMesh = kNode->GetMesh();

    { // create control point & make convert table
        OverlapReducer<_PositionSkin> reducer(pNode->Vertices.Length, [&pNode, &pSkinnedNode](size_t idx)->_PositionSkin{
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
    for(size_t idxLayer = 0; idxLayer < pNode->LayeredElements.Length; ++idxLayer){
        auto* kLayer = kMesh->GetLayer((int)idxLayer);
        if(!kLayer){
            const auto idxNewLayer = kMesh->CreateLayer();
            if(idxNewLayer < 0){
                std::string msg = "failed to create layer";
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }
            else if(idxLayer != idxNewLayer){
                std::string msg = "the created layer has unexpected index number";
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            kLayer = kMesh->GetLayer(idxNewLayer);
        }
    }

    for(size_t idxLayer = 0; idxLayer < pNode->LayeredElements.Length; ++idxLayer){
        const auto& iLayer = pNode->LayeredElements.Values[idxLayer];
        auto* kLayer = kMesh->GetLayer((int)idxLayer);

        if(iLayer.Material.Length){

        }

        if(iLayer.Color.Length){
            auto* kColor = FbxLayerElementVertexColor::Create(kMesh, "");
            if(kColor){
                kColor->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
                kColor->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
            }
            else{
                std::string msg = "failed to create element vertex color in the layer " + std::to_string(idxLayer);
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            OverlapReducer<Float4> reducer(iLayer.Color.Length, [&iLayer](size_t idx)->Float4{
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
            if(kNormal){
                kNormal->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
                kNormal->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
            }
            else{
                std::string msg = "failed to create element normal in the layer " + std::to_string(idxLayer);
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            OverlapReducer<Float3> reducer(iLayer.Normal.Length, [&iLayer](size_t idx)->Float3{
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
            if(kBinormal){
                kBinormal->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
                kBinormal->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
            }
            else{
                std::string msg = "failed to create element binormal in the layer " + std::to_string(idxLayer);
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            OverlapReducer<Float3> reducer(iLayer.Binormal.Length, [&iLayer](size_t idx)->Float3{
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
            if(kTangent){
                kTangent->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
                kTangent->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
            }
            else{
                std::string msg = "failed to create element tangent in the layer " + std::to_string(idxLayer);
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            OverlapReducer<Float3> reducer(iLayer.Tangent.Length, [&iLayer](size_t idx)->Float3{
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
            if(kUV){
                kUV->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
                kUV->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
            }
            else{
                std::string msg = "failed to create element UV in the layer " + std::to_string(idxLayer);
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            OverlapReducer<Float2> reducer(iLayer.Texcoord.Length, [&iLayer](size_t idx)->Float2{
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
