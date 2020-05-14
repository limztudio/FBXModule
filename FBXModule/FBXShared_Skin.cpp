/**
* @file FBXUtilites_Skin.cpp
* @date 2019/04/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <eastl/map.h>

#include "FBXUtilites.h"
#include "FBXShared.h"


static const int ins_maxWeightParticipations = 4;


FbxAMatrix SHRGetBlendMatrix(const SkinData* skins, size_t count){
    FbxAMatrix matOut;

    ZeroMemory(matOut.mData, sizeof(matOut.mData));

    for(const auto* i = skins; (i - skins) < (int)count; ++i){
        auto matRes = Scale44(i->matrix, i->weight);
        CopyMemory(matOut.mData, &matRes.mData, sizeof(matRes.mData));
    }

    return matOut;
}

bool SHRLoadSkinFromNode(const ControlPointRemap& controlPointRemap, FbxNode* kNode, NodeData* pNodeData){
    static const char __name_of_this_func[] = "SHRLoadSkinFromNode(const ControlPointRemap&, FbxNode*, NodeData*)";


    auto* kMesh = (FbxMesh*)kNode->GetNodeAttribute();

    auto skinCount = kMesh->GetDeformerCount(FbxDeformer::eSkin);
    if(!skinCount)
        return true;

    auto* kSkin = static_cast<FbxSkin*>(kMesh->GetDeformer(0, FbxDeformer::eSkin));
    if(!kSkin)
        return true;

    auto clusterCount = kSkin->GetClusterCount();
    if(!clusterCount)
        return true;

    auto& skinTable = pNodeData->bufSkinData;
    auto& boneOffsetMatrixMap = pNodeData->mapBoneDeformMatrices;
    {
        skinTable.resize(pNodeData->bufPositions.size());
    }

    eastl::vector<FbxCluster*> clusterFinder(clusterCount, nullptr);
    eastl::vector<eastl::unordered_map<int, FbxDouble>> boneMapList(clusterCount);
    eastl::vector<eastl::multimap<double, int>> vertexBoneList(pNodeData->bufPositions.size());

    auto ctrlPointCount = kMesh->GetControlPointsCount();

    auto kLinkMode = kSkin->GetCluster(0)->GetLinkMode();
    switch(kLinkMode){
    case FbxCluster::eNormalize:
    case FbxCluster::eTotalOne:
        break;

    default:
        SHRPushErrorMessage("unsupported cluster link mode", __name_of_this_func);
        return false;
    }

    for(auto iCluster = decltype(clusterCount){ 0 }; iCluster < clusterCount; ++iCluster){
        auto* kCluster = kSkin->GetCluster(iCluster);

        if(kLinkMode != kCluster->GetLinkMode()){
            SHRPushErrorMessage("every clusters in skin must have same link mode", __name_of_this_func);
            return false;
        }

        auto* kLinkNode = kCluster->GetLink();
        if(!kLinkNode)
            continue;

        clusterFinder[iCluster] = kCluster;

        auto& vertexBoneWeightMap = boneMapList[iCluster];

        auto indexCount = kCluster->GetControlPointIndicesCount();

        auto* indices = kCluster->GetControlPointIndices();
        auto* weights = kCluster->GetControlPointWeights();

        for(auto iIndex = decltype(indexCount){ 0 }; iIndex < indexCount; ++iIndex){
            auto ctrlPointIndex = indices[iIndex];

            if(ctrlPointIndex >= ctrlPointCount){
                SHRPushErrorMessage("unexpected control point index", __name_of_this_func);
                return false;
            }

            if(!controlPointRemap.empty()){
                for(const auto& iRemap : controlPointRemap[ctrlPointIndex]){
                    auto vid = iRemap;

                    vertexBoneWeightMap[vid] = 0;
                }
            }
            else{
                auto vid = ctrlPointIndex;

                vertexBoneWeightMap[vid] = 0;
            }
        }

        for(auto iIndex = decltype(indexCount){ 0 }; iIndex < indexCount; ++iIndex){
            const auto& weight = weights[iIndex];
            if(!weight)
                continue;

            auto ctrlPointIndex = indices[iIndex];

            if(!controlPointRemap.empty()){
                for(const auto& iRemap : controlPointRemap[ctrlPointIndex]){
                    auto vid = iRemap;

                    vertexBoneWeightMap[vid] += weight;
                }
            }
            else{
                auto vid = ctrlPointIndex;

                vertexBoneWeightMap[vid] += weight;
            }
        }

        for(auto& iVertex : vertexBoneWeightMap){
            auto vid = iVertex.first;

            auto& weightBoneMap = vertexBoneList[vid];
            weightBoneMap.emplace(iVertex.second, iCluster);
        }
    }

    for(int idxBone = 0, edxBone = (decltype(edxBone))boneMapList.size(); idxBone < edxBone; ++idxBone){
        for(const auto& iBone : boneMapList[idxBone]){
            const auto& vertID = iBone.first;
            const auto& vertWeight = iBone.second;

            bool found = false;
            for(const auto& iVert : vertexBoneList[vertID]){
                if(iVert.second == idxBone){
                    found = true;
                    break;
                }
            }

            if(!found){
                SHRPushErrorMessage("validation check failed. bone indices are not matched", __name_of_this_func);
                return false;
            }
        }
    }

    for(int idxVert = 0, edxVert = (decltype(edxVert))vertexBoneList.size(); idxVert < edxVert; ++idxVert){
        auto& weightBoneMap = vertexBoneList[idxVert];

        // remove too many participated clusters
        if((decltype(ins_maxWeightParticipations))weightBoneMap.size() > ins_maxWeightParticipations){
            int counter = 0;
            for(auto itrBone = weightBoneMap.rbegin(); itrBone != weightBoneMap.rend(); ++itrBone){
                auto& vertexBoneWeightMap = boneMapList[itrBone->second];

                auto itrVertBone = vertexBoneWeightMap.find(idxVert);
                if(itrVertBone != vertexBoneWeightMap.end()){
                    if(counter >= ins_maxWeightParticipations)
                        vertexBoneWeightMap.erase(itrVertBone);
                }

                ++counter;
            }

            auto itrBone = weightBoneMap.begin();
            for(int i = 0, e = (decltype(e))weightBoneMap.size() - ins_maxWeightParticipations; i < e; ++i){
                auto itrDel = itrBone;
                ++itrBone;

                weightBoneMap.erase(itrBone);
            }
        }

        double totalWeight = 0;
        { // normalize
            for(const auto& i : weightBoneMap)
                totalWeight += i.first;

            if(totalWeight > 0){
                for(auto& iWeightBone : weightBoneMap){
                    auto& vertexBoneWeightMap = boneMapList[iWeightBone.second];
                    auto itrVertBone = vertexBoneWeightMap.find(idxVert);

                    itrVertBone->second /= totalWeight;
                }
            }
        }

        if(!totalWeight)
            continue;

        for(auto& iWeightBone : weightBoneMap){
            auto* curCluster = clusterFinder[iWeightBone.second];
            if(!curCluster)
                continue;
            
            {
                SkinInfo _emplace = { curCluster, iWeightBone.first };
                skinTable[idxVert].emplace_back(eastl::move(_emplace));
            }
        }
    }

    for(auto* iCluster : clusterFinder){
        if(!iCluster)
            continue;

        FbxAMatrix kTransformMatrix, kTransformLinkMatrix;
        iCluster->GetTransformMatrix(kTransformMatrix);
        iCluster->GetTransformLinkMatrix(kTransformLinkMatrix);

        auto kVertexTransformMatrix = kTransformLinkMatrix.Inverse() * kTransformMatrix;

        boneOffsetMatrixMap.emplace(iCluster, eastl::move(kVertexTransformMatrix));
    }

    return true;
}


bool SHRInitSkinData(FbxManager* kSDKManager, const FBXNodeToFbxNode& nodeBinder, const FBXSkinnedMesh* pNode, FbxNode* kNode){
    static const char __name_of_this_func[] = "SHRInitSkinData(FbxManager*, const FBXNodeToFbxNode&, const FBXSkinnedMesh*, FbxNode*)";


    const eastl::string strName = pNode->Name;

    auto* kMesh = kNode->GetMesh();
    auto* kSkin = kMesh->GetDeformer(0);

    for(const auto* pDeform = pNode->SkinDeforms.Values; FBX_PTRDIFFU(pDeform - pNode->SkinDeforms.Values) < pNode->SkinDeforms.Length; ++pDeform){
        if(!pDeform->TargetNode){
            eastl::string msg = "skin deformer must have value not null";
            msg += "(errored in \"";
            msg += strName;
            msg += "\")";
            SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
            continue;
        }

        FbxNode* kTargetNode = nullptr;
        {
            auto f = nodeBinder.find(pDeform->TargetNode);
            if(f == nodeBinder.cend()){
                eastl::string msg = "failed to find bind node of ";
                msg += pDeform->TargetNode->Name;
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                continue;
            }

            kTargetNode = f->second;
        }

        auto* kCluster = FbxCluster::Create(kSDKManager, "");
        if(!kCluster){
            eastl::string msg = "failed to create FbxCluster";
            msg += "(errored in \"";
            msg += strName;
            msg += "\")";
            SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
            continue;
        }

        kCluster->SetLink(kTargetNode);
        kCluster->SetLinkMode(FbxCluster::eTotalOne);

        auto kMatTransform = GetGlobalTransform(kTargetNode);
        kCluster->SetTransformMatrix(kMatTransform);

        FbxAMatrix kMatDeform;
        CopyArrayData<pDeform->DeformMatrix.Length>((double*)kMatDeform, pDeform->DeformMatrix.Values);

        auto kMatLink = kMatTransform * kMatDeform.Inverse();
        kCluster->SetTransformLinkMatrix(kMatLink);

        //kCluster->AddControlPointIndex();
    }

    return true;
}
