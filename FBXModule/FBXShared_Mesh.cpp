/**
* @file FBXUtilites_Mesh.cpp
* @date 2019/04/12
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include "FBXUtilites.h"
#include "FBXShared.h"


template<typename LHS, typename RHS>
static inline void ins_copyData(LHS& lhs, const RHS& rhs){
    lhs = static_cast<LHS>(rhs);
}
template<typename LHS_TYPE, typename RHS>
static inline void ins_copyData(FbxVectorTemplate2<LHS_TYPE>& lhs, const RHS& rhs){
    for(int i = 0, e = (decltype(e))_countof(lhs.mData); i < e; ++i)
        lhs[i] = static_cast<LHS_TYPE>(rhs[i]);
}
template<typename LHS_TYPE, typename RHS>
static inline void ins_copyData(FbxVectorTemplate3<LHS_TYPE>& lhs, const RHS& rhs){
    for(int i = 0, e = (decltype(e))_countof(lhs.mData); i < e; ++i)
        lhs[i] = static_cast<LHS_TYPE>(rhs[i]);
}
template<typename LHS_TYPE, typename RHS>
static inline void ins_copyData(FbxVectorTemplate4<LHS_TYPE>& lhs, const RHS& rhs){
    for(int i = 0, e = (decltype(e))_countof(lhs.mData); i < e; ++i)
        lhs[i] = static_cast<LHS_TYPE>(rhs[i]);
}


bool SHRLoadMeshFromNode(ControlPointRemap& controlPointRemap, FbxNode* kNode, NodeData* pNodeData){
    static const char __name_of_this_func[] = "SHRLoadMeshFromNode(FbxNode*, ControlPointRemap&)";


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
                SHRPushErrorMessage("vertex index and control point index are not matched", __name_of_this_func);
                return false;
            }
        }
    }

    controlPointRemap.clear();
    controlPointRemap.resize(ctrlPointsCount);

    eastl::vector<int> flatIndexToCtrlPoint(polyCount * 3);

    int t = 0;
    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
        auto polySize = kMesh->GetPolygonSize(iPoly);

        if(polySize != 3){
            SHRPushErrorMessage("polygon size must be 3", __name_of_this_func);
            return false;
        }

        for(auto iVert = decltype(polySize){ 0 }; iVert < 3; ++iVert){
            auto ctrlPointIndex = kMesh->GetPolygonVertex(iPoly, iVert);

            if(ctrlPointIndex < 0){
                SHRPushErrorMessage("vertex index must be bigger than or equal to 0", __name_of_this_func);
                return false;
            }

            auto flatIndex = iPoly * 3 + iVert;
            if(t != flatIndex){
                SHRPushErrorMessage("vertex index is invalid", __name_of_this_func);
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
        auto& edges = pNodeData->bufEdges;

        auto& layers = pNodeData->bufLayers;

        { // reserve
            {
                positions.resize(polyCount * 3);
                indices.resize(polyCount);
                edges.resize(polyCount);

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

        {
            for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                const auto& curIndex = indices[iPoly];
                auto& curEdgeTable = edges[iPoly];

                for(int iVert = 0; iVert < 3; ++iVert){
                    int iVertNext = (iVert + 1) % 3;

                    const auto vCtrlBegin = flatIndexToCtrlPoint[curIndex.raw[iVert]];
                    const auto vCtrlEnd = flatIndexToCtrlPoint[curIndex.raw[iVertNext]];

                    int edgeIndex;
                    {
                        bool bReserved;
                        edgeIndex = kMesh->GetMeshEdgeIndex(vCtrlBegin, vCtrlEnd, bReserved);
                        if(edgeIndex == -1)
                            edgeIndex = kMesh->GetMeshEdgeIndex(vCtrlEnd, vCtrlBegin, bReserved);
                    }

                    curEdgeTable.raw[iVert] = edgeIndex;
                }
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

            const auto* kSmoothings = kLayer->GetSmoothing();
            const auto* kMaterials = kLayer->GetMaterials();

            const auto* kVertColors = kLayer->GetVertexColors();
            const auto* kTexUVs = kLayer->GetUVs();
            const auto* kNormals = kLayer->GetNormals();
            const auto* kBinormals = kLayer->GetBinormals();
            const auto* kTangents = kLayer->GetTangents();

            auto& pLayer = layers[iLayer];

            if(kSmoothings){
                auto& pObject = pLayer.smoothings;
                auto* kObject = kSmoothings;

                pObject.resize(polyCount, { -1, -1, -1 });

                switch(kObject->GetMappingMode()){
                case FbxLayerElement::eByEdge:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        const auto& curEdgeTable = edges[iPoly];

                        for(int iEdge = 0; iEdge < 3; ++iEdge){
                            auto edgeIndex = curEdgeTable.raw[iEdge];

                            auto& edge = pObject[iPoly].raw[iEdge];

                            switch(kObject->GetReferenceMode()){
                            case FbxLayerElement::eDirect:
                                edge = kObject->GetDirectArray().GetAt(edgeIndex);
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                edge = kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(edgeIndex));
                                break;

                            default:
                                SHRPushErrorMessage("smoothing has unsupported reference mode", __name_of_this_func);
                                return false;
                            }
                        }
                    }
                    break;

                case FbxLayerElement::eByPolygon:
                    for(auto iPoly = decltype(polyCount){ 0 }; iPoly < polyCount; ++iPoly){
                        int smoothIndex;

                        switch(kObject->GetReferenceMode()){
                        case FbxLayerElement::eDirect:
                            smoothIndex = kObject->GetDirectArray().GetAt(iPoly);
                            break;

                        case FbxLayerElement::eIndexToDirect:
                            smoothIndex = kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(iPoly));
                            break;

                        default:
                            SHRPushErrorMessage("smoothing has unsupported reference mode", __name_of_this_func);
                            return false;
                        }

                        auto& poly = pObject[iPoly];
                        for(auto& i : poly.raw)
                            i = smoothIndex;
                    }
                    break;

                default:
                    SHRPushErrorMessage("smoothing has unsupported mapping mode", __name_of_this_func);
                    return false;
                }
            }

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
                    SHRPushErrorMessage("material has unsupported mapping mode", __name_of_this_func);
                    return false;
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
                                ins_copyData(vert, kObject->GetDirectArray().GetAt(ctrlPointIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                ins_copyData(vert, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(ctrlPointIndex)));
                                break;

                            default:
                                SHRPushErrorMessage("vertex color has unsupported reference mode", __name_of_this_func);
                                return false;
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
                                ins_copyData(vert, kObject->GetDirectArray().GetAt(flatIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                ins_copyData(vert, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(flatIndex)));
                                break;

                            default:
                                SHRPushErrorMessage("vertex color has unsupported reference mode", __name_of_this_func);
                                return false;
                            }
                        }
                    }
                    break;

                default:
                    SHRPushErrorMessage("vertex color has unsupported mapping mode", __name_of_this_func);
                    return false;
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
                                ins_copyData(vert, kObject->GetDirectArray().GetAt(ctrlPointIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                ins_copyData(vert, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(ctrlPointIndex)));
                                break;

                            default:
                                SHRPushErrorMessage("vertex color has unsupported reference mode", __name_of_this_func);
                                return false;
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
                                ins_copyData(vert, kObject->GetDirectArray().GetAt(uvIndex));
                                break;

                            default:
                                SHRPushErrorMessage("vertex color has unsupported reference mode", __name_of_this_func);
                                return false;
                            }

                            vert[1] = 1. - vert[1];
                        }
                    }
                    break;

                default:
                    SHRPushErrorMessage("texcoord has unsupported mapping mode", __name_of_this_func);
                    return false;
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
                                ins_copyData(vert, kObject->GetDirectArray().GetAt(flatIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                ins_copyData(vert, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(flatIndex)));
                                break;

                            default:
                                SHRPushErrorMessage("vertex normal has unsupported reference mode", __name_of_this_func);
                                return false;
                            }

                            vert = Transform33(kRefGeometry, vert);
                            vert = Normalize3(vert);
                        }
                    }
                    break;

                default:
                    SHRPushErrorMessage("vertex normal has unsupported mapping mode", __name_of_this_func);
                    return false;
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
                                ins_copyData(vert, kObject->GetDirectArray().GetAt(flatIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                ins_copyData(vert, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(flatIndex)));
                                break;

                            default:
                                SHRPushErrorMessage("vertex binormal has unsupported reference mode", __name_of_this_func);
                                return false;
                            }

                            vert = Transform33(kRefGeometry, vert);
                            vert = Normalize3(vert);
                        }
                    }
                    break;

                default:
                    SHRPushErrorMessage("vertex binormal has unsupported mapping mode", __name_of_this_func);
                    return false;
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
                                ins_copyData(vert, kObject->GetDirectArray().GetAt(flatIndex));
                                break;

                            case FbxLayerElement::eIndexToDirect:
                                ins_copyData(vert, kObject->GetDirectArray().GetAt(kObject->GetIndexArray().GetAt(flatIndex)));
                                break;

                            default:
                                SHRPushErrorMessage("vertex tangent has unsupported reference mode", __name_of_this_func);
                                return false;
                            }

                            vert = Transform33(kRefGeometry, vert);
                            vert = Normalize3(vert);
                        }
                    }
                    break;

                default:
                    SHRPushErrorMessage("vertex tangent has unsupported mapping mode", __name_of_this_func);
                    return false;
                }
            }
        }
    }

    return true;
}

bool SHRInitMeshNode(FbxManager* kSDKManager, const FBXMesh* pNode, FbxNode* kNode){
    auto* kMesh = kNode->GetMesh();

    return true;
}
bool SHRInitSkinnedMeshNode(FbxManager* kSDKManager, const FBXSkinnedMesh* pNode, FbxNode* kNode){
    auto* kMesh = kNode->GetMesh();
    auto* kSkin = kMesh->GetDeformer(0);

    return true;
}