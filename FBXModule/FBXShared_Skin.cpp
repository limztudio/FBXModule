/**
* @file FBXUtilites_Skin.cpp
* @date 2019/04/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <eastl/map.h>
#include <eastl/vector.h>

#include "FBXUtilites.h"
#include "FBXShared.h"


static eastl::vector<eastl::vector<int>> ins_newToOldIndexer;


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
        if(weightBoneMap.size() > shr_ioSetting.MaxParticipateClusterPerVertex){
            size_t counter = 0;
            for(auto itrBone = weightBoneMap.rbegin(); itrBone != weightBoneMap.rend(); ++itrBone){
                auto& vertexBoneWeightMap = boneMapList[itrBone->second];

                auto itrVertBone = vertexBoneWeightMap.find(idxVert);
                if(itrVertBone != vertexBoneWeightMap.end()){
                    if(counter >= shr_ioSetting.MaxParticipateClusterPerVertex)
                        vertexBoneWeightMap.erase(itrVertBone);
                }

                ++counter;
            }

            auto itrBone = weightBoneMap.begin();
            for(size_t i = 0, e = weightBoneMap.size() - shr_ioSetting.MaxParticipateClusterPerVertex; i < e; ++i){
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


bool SHRInitSkinData(FbxManager* kSDKManager, PoseNodeList& poseNodeList, const ImportNodeToFbxNode& nodeBinder, const ControlPointMergeMap& ctrlPointMergeMap, const FBXSkinnedMesh* pNode, FbxNode* kNode){
    static const char __name_of_this_func[] = "SHRInitSkinData(FbxManager*, PoseNodeList&, const FBXNodeToFbxNode&, const ControlPointMergeMap&, const FBXSkinnedMesh*, FbxNode*)";


    const eastl::string strName = pNode->Name.Values;

    eastl::unordered_map<const FBXNode*, FbxCluster*> clusterFinder;
    eastl::unordered_map<const FBXNode*, float> tmpSkinTable;

    auto* kMesh = kNode->GetMesh();

    auto* kSkin = FbxSkin::Create(kSDKManager, "");
    if(!kSkin){
        eastl::string msg = "failed to create FbxSkin";
        msg += "(errored in \"";
        msg += strName;
        msg += "\")";
        SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
        return false;
    }

    ins_newToOldIndexer.clear();
    ins_newToOldIndexer.resize((size_t)kMesh->GetControlPointsCount());
    for(int idxOld = 0, edxOld = (int)ctrlPointMergeMap.size(); idxOld < edxOld; ++idxOld){
        const auto idxNew = (int)ctrlPointMergeMap[idxOld];

        auto& iTable = ins_newToOldIndexer[idxNew];
        iTable.emplace_back(idxOld);
    }
    for(const auto& i : ins_newToOldIndexer){
        if(i.empty()){
            eastl::string msg = "an error occurred while creating control point remapper";
            msg += "(errored in \"";
            msg += strName;
            msg += "\")";
            SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
            return false;
        }
    }

    for(const auto* pDeform = pNode->SkinDeforms.Values; FBX_PTRDIFFU(pDeform - pNode->SkinDeforms.Values) < pNode->SkinDeforms.Length; ++pDeform){
        if(!pDeform->TargetNode){
            eastl::string msg = "skin deformer must have value not null";
            msg += "(errored in \"";
            msg += strName;
            msg += "\")";
            SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
            return false;
        }

        FbxNode* kTargetNode = nullptr;
        {
            auto f = nodeBinder.find(pDeform->TargetNode);
            if(f == nodeBinder.cend()){
                eastl::string msg = "failed to find bind node of ";
                msg += pDeform->TargetNode->Name.Values;
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                return false;
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
            return false;
        }

        kCluster->SetLink(kTargetNode);
        kCluster->SetLinkMode(FbxCluster::eTotalOne);

        auto kMatTransform = GetGlobalTransform(kTargetNode);
        kCluster->SetTransformMatrix(kMatTransform);

        FbxAMatrix kMatDeform;
        CopyArrayData<pDeform->DeformMatrix.Length>((double*)kMatDeform, pDeform->DeformMatrix.Values);

        auto kMatLink = kMatTransform * kMatDeform.Inverse();
        kCluster->SetTransformLinkMatrix(kMatLink);

        if(!kSkin->AddCluster(kCluster)){
            eastl::string msg = "failed to add cluster";
            msg += "(errored in \"";
            msg += strName;
            msg += "\")";
            SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
            return false;
        }

        clusterFinder.emplace(pDeform->TargetNode, kCluster);
        poseNodeList.emplace(kTargetNode);
    }

    for(int idxVert = 0, edxVert = (int)ins_newToOldIndexer.size(); idxVert < edxVert; ++idxVert){
        tmpSkinTable.clear();

        for(const auto idxOld : ins_newToOldIndexer[idxVert]){
            const auto& iVert = pNode->SkinInfos.Values[idxOld];

            for(const auto* iCluster = iVert.Values; FBX_PTRDIFFU(iCluster - iVert.Values) < iVert.Length; ++iCluster){
                auto f = tmpSkinTable.find(iCluster->BindNode);
                if(f != tmpSkinTable.end()){
                    if(iCluster->Weight > f->second)
                        f->second = iCluster->Weight;
                }
                else
                    tmpSkinTable.emplace(iCluster->BindNode, iCluster->Weight);
            }
        }

        float totalWeight = 0.f;
        for(const auto& iCluster : tmpSkinTable)
            totalWeight += iCluster.second;
        for(auto& iCluster : tmpSkinTable)
            iCluster.second /= totalWeight;

        for(const auto& iCluster : tmpSkinTable){
            FbxCluster* kCluster = nullptr;
            {
                auto f = clusterFinder.find(iCluster.first);
                if(f == clusterFinder.end()){
                    eastl::string msg = "failed to link cluster";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
                    SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                    return false;
                }

                kCluster = f->second;
            }

            kCluster->AddControlPointIndex(idxVert, iCluster.second);
        }
    }

    if(kMesh->AddDeformer(kSkin) < 0){
        eastl::string msg = "an error occurred while adding deformer";
        msg += "(errored in \"";
        msg += strName;
        msg += "\")";
        SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
        return false;
    }

    poseNodeList.emplace(kNode);

    return true;
}
