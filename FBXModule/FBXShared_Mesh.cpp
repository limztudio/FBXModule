/**
* @file FBXUtilites_Mesh.cpp
* @date 2019/04/12
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include "FBXUtilites.h"
#include "FBXShared.h"


bool SHRLoadMeshFromNode(ControlPointRemap& controlPointRemap, FbxNode* kNode, NodeData* pNodeData){
    static const char __name_of_this_func[] = "SHRLoadMeshFromNode(ControlPointRemap&, FbxNode*, NodeData*)";


    const eastl::string strName = kNode->GetName();

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
                eastl::string msg = "vertex index and control point index are not matched";
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
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
            eastl::string msg = "polygon size must be 3";
            msg += "(errored in \"";
            msg += strName;
            msg += "\")";
            SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
            return false;
        }

        for(auto iVert = decltype(polySize){ 0 }; iVert < 3; ++iVert){
            auto ctrlPointIndex = kMesh->GetPolygonVertex(iPoly, iVert);

            if(ctrlPointIndex < 0){
                eastl::string msg = "vertex index must be bigger than or equal to 0";
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                return false;
            }

            auto flatIndex = iPoly * 3 + iVert;
            if(t != flatIndex){
                eastl::string msg = "vertex index is invalid";
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
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
                    eastl::string msg = "material has unsupported mapping mode";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
                    SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
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
                                eastl::string msg = "vertex color has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
                                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
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
                                eastl::string msg = "vertex color has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
                                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                                return false;
                            }
                            }
                        }
                    }
                    break;

                default:
                {
                    eastl::string msg = "vertex color has unsupported mapping mode";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
                    SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
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
                                eastl::string msg = "vertex color has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
                                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
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
                                eastl::string msg = "vertex color has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
                                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                                return false;
                            }
                            }

                            vert[1] = 1. - vert[1];
                        }
                    }
                    break;

                default:
                {
                    eastl::string msg = "texcoord has unsupported mapping mode";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
                    SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
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
                                eastl::string msg = "vertex normal has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
                                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
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
                    eastl::string msg = "vertex normal has unsupported mapping mode";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
                    SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
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
                                eastl::string msg = "vertex binormal has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
                                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
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
                    eastl::string msg = "vertex binormal has unsupported mapping mode";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
                    SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
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
                                eastl::string msg = "vertex tangent has unsupported reference mode";
                                msg += "(errored in \"";
                                msg += strName;
                                msg += "\")";
                                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
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
                    eastl::string msg = "vertex tangent has unsupported mapping mode";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
                    SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                    return false;
                }
                }
            }
        }
    }

    return true;
}

bool SHRInitMeshNode(FbxManager* kSDKManager, const FBXMesh* pNode, FbxNode* kNode){
    static const char __name_of_this_func[] = "SHRInitMeshNode(FbxManager*, const FBXMesh*, FbxNode*)";


    const eastl::string strName = pNode->Name;
    auto* kMesh = kNode->GetMesh();

    {
        kMesh->SetControlPointCount((int)pNode->Vertices.Length);

        auto* kPoints = kMesh->GetControlPoints();
        for(auto* pVert = pNode->Vertices.Values; FBX_PTRDIFFU(pVert - pNode->Vertices.Values) < pNode->Vertices.Length; ++pVert, ++kPoints){
            CopyArrayData<pVert->Length>(kPoints->mData, pVert->Values);
            (*kPoints)[3] = 1.;
        }
    }

    {
        for(auto* pPoly = pNode->Indices.Values; FBX_PTRDIFFU(pPoly - pNode->Indices.Values) < pNode->Indices.Length; ++pPoly){
            kMesh->BeginPolygon();

            for(const auto& iPolyIndex : pPoly->Values)
                kMesh->AddPolygon(iPolyIndex);

            kMesh->EndPolygon();
        }
    }

    int iLayer = 0;
    for(auto* pLayer = pNode->LayeredVertices.Values; FBX_PTRDIFFU(pLayer - pNode->LayeredVertices.Values) < pNode->LayeredVertices.Length; ++pLayer, ++iLayer){
        auto* kLayer = kMesh->GetLayer(iLayer);
        if(!kLayer){
            const auto iNewLayer = kMesh->CreateLayer();
            if(iNewLayer < 0){
                eastl::string msg = "failed to create layer";
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                continue;
            }
            else if(iLayer != iNewLayer){
                eastl::string msg = "the created layer has unexpected index number";
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                continue;
            }
            
            kLayer = kMesh->GetLayer(iNewLayer);
        }

        //const auto layerName = "Layer" + eastl::to_string(FBX_PTRDIFFU(pLayer - pNode->LayeredVertices.Values));

        if(pLayer->Material.Length){
            //
        }

        if(pLayer->Color.Length){
            auto* kColor = FbxLayerElementVertexColor::Create(kMesh, "");
            if(kColor){
                kColor->SetMappingMode(FbxGeometryElement::eByControlPoint);
                kColor->SetReferenceMode(FbxGeometryElement::eDirect);

                auto& kArray = kColor->GetDirectArray();
                for(auto* pVert = pLayer->Color.Values; FBX_PTRDIFFU(pVert - pLayer->Color.Values) < pLayer->Color.Length; ++pVert){
                    FbxVector4 kValue;

                    CopyArrayData<pVert->Length>(kValue.Buffer(), pVert->Values);

                    kArray.Add(kValue);
                }

                kLayer->SetVertexColors(kColor);
            }
            else{
                eastl::string msg = "failed to create element vertex color in the layer " + eastl::to_string(iLayer);
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
            }
        }

        if(pLayer->Normal.Length){
            auto* kNormal = FbxLayerElementNormal::Create(kMesh, "");
            if(kNormal){
                kNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
                kNormal->SetReferenceMode(FbxGeometryElement::eDirect);

                auto& kArray = kNormal->GetDirectArray();
                for(auto* pVert = pLayer->Normal.Values; FBX_PTRDIFFU(pVert - pLayer->Normal.Values) < pLayer->Normal.Length; ++pVert){
                    FbxVector4 kValue;

                    CopyArrayData<pVert->Length>(kValue.Buffer(), pVert->Values);
                    kValue[3] = 1.;
                    kValue.Normalize();

                    kArray.Add(kValue);
                }

                kLayer->SetNormals(kNormal);
            }
            else{
                eastl::string msg = "failed to create element normal in the layer " + eastl::to_string(iLayer);
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
            }
        }
        if(pLayer->Binormal.Length){
            auto* kBinormal = FbxLayerElementBinormal::Create(kMesh, "");
            if(kBinormal){
                kBinormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
                kBinormal->SetReferenceMode(FbxGeometryElement::eDirect);

                auto& kArray = kBinormal->GetDirectArray();
                for(auto* pVert = pLayer->Binormal.Values; FBX_PTRDIFFU(pVert - pLayer->Binormal.Values) < pLayer->Binormal.Length; ++pVert){
                    FbxVector4 kValue;

                    CopyArrayData<pVert->Length>(kValue.Buffer(), pVert->Values);
                    kValue[3] = 1.;
                    kValue.Normalize();

                    kArray.Add(kValue);
                }

                kLayer->SetBinormals(kBinormal);
            }
            else{
                eastl::string msg = "failed to create element binormal in the layer " + eastl::to_string(iLayer);
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
            }
        }
        if(pLayer->Tangent.Length){
            auto* kTangent = FbxLayerElementTangent::Create(kMesh, "");
            if(kTangent){
                kTangent->SetMappingMode(FbxGeometryElement::eByControlPoint);
                kTangent->SetReferenceMode(FbxGeometryElement::eDirect);

                auto& kArray = kTangent->GetDirectArray();
                for(auto* pVert = pLayer->Tangent.Values; FBX_PTRDIFFU(pVert - pLayer->Tangent.Values) < pLayer->Tangent.Length; ++pVert){
                    FbxVector4 kValue;

                    CopyArrayData<pVert->Length>(kValue.Buffer(), pVert->Values);
                    kValue[3] = 1.;
                    kValue.Normalize();

                    kArray.Add(kValue);
                }

                kLayer->SetTangents(kTangent);
            }
            else{
                eastl::string msg = "failed to create element tangent in the layer " + eastl::to_string(iLayer);
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
            }
        }

        if(pLayer->Texcoord.Length){
            auto* kUV = FbxLayerElementUV::Create(kMesh, "");
            if(kUV){
                kUV->SetMappingMode(FbxGeometryElement::eByControlPoint);
                kUV->SetReferenceMode(FbxGeometryElement::eDirect);

                auto& kArray = kUV->GetDirectArray();
                for(auto* pVert = pLayer->Texcoord.Values; FBX_PTRDIFFU(pVert - pLayer->Texcoord.Values) < pLayer->Texcoord.Length; ++pVert){
                    FbxVector2 kValue;

                    CopyArrayData<pVert->Length>(kValue.Buffer(), pVert->Values);
                    kValue[1] = 1. - kValue[1];

                    kArray.Add(kValue);
                }

                kLayer->SetUVs(kUV);
            }
            else{
                eastl::string msg = "failed to create element UV in the layer " + eastl::to_string(iLayer);
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
            }
        }
    }

    return true;
}
