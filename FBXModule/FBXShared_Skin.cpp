/**
 * @file FBXUtilites_Skin.cpp
 * @date 2019/04/15
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#include "stdafx.h"

#include <map>

#include "FBXUtilites.h"
#include "FBXMath.h"
#include "FBXShared.h"


static fbx_vector<fbx_vector<unsigned int>> ins_newToOldIndexer;


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
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRLoadSkinFromNode(const ControlPointRemap&, FbxNode*, NodeData*)");


    auto* kMesh = (FbxMesh*)kNode->GetNodeAttribute();

    auto skinCount = kMesh->GetDeformerCount(FbxDeformer::eSkin);
    if(!skinCount)
        return true;

    auto* kSkin = static_cast<FbxSkin*>(kMesh->GetDeformer(0, FbxDeformer::eSkin));
    if(!kSkin)
        return true;

    auto clusterCount = (unsigned int)kSkin->GetClusterCount();
    if(!clusterCount)
        return true;

    auto& skinTable = pNodeData->bufSkinData;
    auto& boneOffsetMatrixMap = pNodeData->mapBoneDeformMatrices;
    {
        skinTable.resize(pNodeData->bufPositions.size());
    }

    fbx_vector<FbxCluster*> clusterFinder(clusterCount, nullptr);
    fbx_vector<fbx_unordered_map<unsigned int, FbxDouble>> boneMapList(clusterCount);
    fbx_vector<std::multimap<double, unsigned int>> vertexBoneList(pNodeData->bufPositions.size());

    const auto ctrlPointCount = (unsigned int)kMesh->GetControlPointsCount();

    auto kLinkMode = kSkin->GetCluster(0)->GetLinkMode();
    switch(kLinkMode){
    case FbxCluster::eNormalize:
    case FbxCluster::eTotalOne:
        break;

    default:
        SHRPushErrorMessage(FBX_TEXT("unsupported cluster link mode"), __name_of_this_func);
        return false;
    }

    for(auto iCluster = decltype(clusterCount){ 0u }; iCluster < clusterCount; ++iCluster){
        auto* kCluster = kSkin->GetCluster(iCluster);

        if(kLinkMode != kCluster->GetLinkMode()){
            SHRPushErrorMessage(FBX_TEXT("every clusters in skin must have same link mode"), __name_of_this_func);
            return false;
        }

        auto* kLinkNode = kCluster->GetLink();
        if(!kLinkNode)
            continue;

        clusterFinder[iCluster] = kCluster;

        auto& vertexBoneWeightMap = boneMapList[iCluster];

        auto indexCount = (unsigned int)kCluster->GetControlPointIndicesCount();

        auto* indices = kCluster->GetControlPointIndices();
        auto* weights = kCluster->GetControlPointWeights();

        for(auto iIndex = decltype(indexCount){ 0u }; iIndex < indexCount; ++iIndex){
            auto ctrlPointIndex = (unsigned int)indices[iIndex];

            if(ctrlPointIndex >= ctrlPointCount){
                SHRPushErrorMessage(FBX_TEXT("unexpected control point index"), __name_of_this_func);
                return false;
            }

            if(!controlPointRemap.empty()){
                for(const auto& iRemap : controlPointRemap[ctrlPointIndex]){
                    auto vid = iRemap;

                    vertexBoneWeightMap[vid] = 0.;
                }
            }
            else{
                auto vid = ctrlPointIndex;

                vertexBoneWeightMap[vid] = 0.;
            }
        }

        for(auto iIndex = decltype(indexCount){ 0u }; iIndex < indexCount; ++iIndex){
            const auto& weight = weights[iIndex];
            if(!weight)
                continue;

            auto ctrlPointIndex = (unsigned int)indices[iIndex];

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

    for(auto edxBone = (unsigned int)boneMapList.size(), idxBone = 0u; idxBone < edxBone; ++idxBone){
        for(const auto& iBone : boneMapList[idxBone]){
            const auto& vertID = iBone.first;

            bool found = false;
            for(const auto& iVert : vertexBoneList[vertID]){
                if(iVert.second == idxBone){
                    found = true;
                    break;
                }
            }

            if(!found){
                SHRPushErrorMessage(FBX_TEXT("validation check failed. bone index is not match"), __name_of_this_func);
                return false;
            }
        }
    }

    for(auto edxVert = (unsigned int)vertexBoneList.size(), idxVert = 0u; idxVert < edxVert; ++idxVert){
        auto& weightBoneMap = vertexBoneList[idxVert];

        // remove too many participated clusters
        while(weightBoneMap.size() > shr_ioSetting.MaxParticipateClusterPerVertex){
            auto itrBone = weightBoneMap.begin();
            auto& vertexBoneWeightMap = boneMapList[itrBone->second];
            auto itrVertBone = vertexBoneWeightMap.find(idxVert);

            weightBoneMap.erase(itrBone);

            if(itrVertBone != vertexBoneWeightMap.end())
                vertexBoneWeightMap.erase(itrVertBone);
        }

        double totalWeight = 0.;
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
                skinTable[idxVert].emplace_back(std::move(_emplace));
            }
        }
    }

    for(auto* iCluster : clusterFinder){
        if(!iCluster)
            continue;

        auto kMatClusterGeometry = GetGeometry(iCluster->GetLink());

        FbxAMatrix kMatNodeTM, kMatClusterTM;
        iCluster->GetTransformMatrix(kMatNodeTM);
        iCluster->GetTransformLinkMatrix(kMatClusterTM);

        kMatClusterTM *= kMatClusterGeometry;

        boneOffsetMatrixMap.emplace(iCluster, std::make_pair(std::move(kMatNodeTM), std::move(kMatClusterTM)));
    }

    return true;
}


bool SHRInitSkinData(FbxManager* kSDKManager, PoseNodeList& poseNodeList, const ImportNodeToFbxNode& nodeBinder, const ControlPointMergeMap& ctrlPointMergeMap, const FBXSkinnedMesh* pNode, FbxNode* kNode){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRInitSkinData(FbxManager*, PoseNodeList&, const FBXNodeToFbxNode&, const ControlPointMergeMap&, const FBXSkinnedMesh*, FbxNode*)");


    const fbx_string strName = pNode->Name.Values;

    fbx_unordered_map<const FBXNode*, FbxCluster*, PointerHasher<const FBXNode*>> clusterFinder;
    fbx_unordered_map<const FBXNode*, float, PointerHasher<const FBXNode*>> tmpSkinTable;

    auto* kMesh = kNode->GetMesh();

    auto* kSkin = FbxSkin::Create(kSDKManager, "");
    if(!kSkin){
        fbx_string msg = FBX_TEXT("failed to create FbxSkin");
        msg += FBX_TEXT("(errored in \"");
        msg += strName;
        msg += FBX_TEXT("\")");
        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
        return false;
    }

    ins_newToOldIndexer.clear();
    ins_newToOldIndexer.resize((size_t)kMesh->GetControlPointsCount());
    for(auto edxOld = (unsigned int)ctrlPointMergeMap.size(), idxOld = 0u; idxOld < edxOld; ++idxOld){
        const auto idxNew = ctrlPointMergeMap[idxOld];

        auto& iTable = ins_newToOldIndexer[idxNew];
        iTable.emplace_back(idxOld);
    }
    for(const auto& i : ins_newToOldIndexer){
        if(i.empty()){
            fbx_string msg = FBX_TEXT("an error occurred while creating control point remapper");
            msg += FBX_TEXT("(errored in \"");
            msg += strName;
            msg += FBX_TEXT("\")");
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return false;
        }
    }

    if(!pNode->SkinDeforms.Length){
        fbx_unordered_map<FbxNode*, const FBXNode*, PointerHasher<FbxNode*>> bindedNodes;

        for(const auto* pSkinInfos = pNode->SkinInfos.Values; FBX_PTRDIFFU(pSkinInfos - pNode->SkinInfos.Values) < pNode->SkinInfos.Length; ++pSkinInfos){
            for(const auto* pSkinInfo = pSkinInfos->Values; FBX_PTRDIFFU(pSkinInfo - pSkinInfos->Values) < pSkinInfos->Length; ++pSkinInfo){
                const auto* pBindNode = pSkinInfo->BindNode;

                if(!pBindNode){
                    fbx_string msg = FBX_TEXT("skin info must have value not null");
                    msg += FBX_TEXT("(errored in \"");
                    msg += strName;
                    msg += FBX_TEXT("\")");
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                    return false;
                }

                FbxNode* kBindNode = nullptr;
                {
                    auto f = nodeBinder.find(pBindNode);
                    if(f == nodeBinder.cend()){
                        fbx_string msg = FBX_TEXT("failed to find bind node of ");
                        msg += pBindNode->Name.Values;
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    kBindNode = f->second;
                }

                if(kBindNode)
                    bindedNodes.emplace(kBindNode, pBindNode);
            }
        }

        for(const auto& iBindNode : bindedNodes){
            const auto* pBindNode = iBindNode.second;
            auto* kBindNode = iBindNode.first;

            auto* kCluster = FbxCluster::Create(kSDKManager, "");
            if(!kCluster){
                fbx_string msg = FBX_TEXT("failed to create FbxCluster");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            kCluster->SetLink(kBindNode);
            kCluster->SetLinkMode(FbxCluster::eTotalOne);

            auto kMatTransform = GetGlobalTransform(kNode);
            kCluster->SetTransformMatrix(kMatTransform);

            auto kMatLink = GetGlobalTransform(kBindNode);
            kCluster->SetTransformLinkMatrix(kMatLink);

            if(!kSkin->AddCluster(kCluster)){
                fbx_string msg = FBX_TEXT("failed to add cluster");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            clusterFinder.emplace(pBindNode, kCluster);

            for(auto* kParentNode = kBindNode->GetParent(); (kParentNode && kParentNode->GetParent()); kParentNode = kParentNode->GetParent())
                poseNodeList.emplace(kParentNode);
            poseNodeList.emplace(kBindNode);
        }
    }
    else{
        for(const auto* pDeform = pNode->SkinDeforms.Values; FBX_PTRDIFFU(pDeform - pNode->SkinDeforms.Values) < pNode->SkinDeforms.Length; ++pDeform){
            const auto* pTargetNode = pDeform->TargetNode;

            if(!pTargetNode){
                fbx_string msg = FBX_TEXT("skin deformer must have value not null");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            FbxNode* kTargetNode = nullptr;
            {
                auto f = nodeBinder.find(pTargetNode);
                if(f == nodeBinder.cend()){
                    fbx_string msg = FBX_TEXT("failed to find target node of ");
                    msg += pTargetNode->Name.Values;
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                    return false;
                }

                kTargetNode = f->second;
            }

            auto* kCluster = FbxCluster::Create(kSDKManager, "");
            if(!kCluster){
                fbx_string msg = FBX_TEXT("failed to create FbxCluster");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            kCluster->SetLink(kTargetNode);
            kCluster->SetLinkMode(FbxCluster::eTotalOne);

            FbxAMatrix kMatDeformTrans;
            CopyArrayData<pDeform->TransformMatrix.Length>((double*)kMatDeformTrans, pDeform->TransformMatrix.Values);

            FbxAMatrix kMatDeformLink;
            CopyArrayData<pDeform->LinkMatrix.Length>((double*)kMatDeformLink, pDeform->LinkMatrix.Values);

            kCluster->SetTransformMatrix(kMatDeformTrans);
            kCluster->SetTransformLinkMatrix(kMatDeformLink);

            if(!kSkin->AddCluster(kCluster)){
                fbx_string msg = FBX_TEXT("failed to add cluster");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            clusterFinder.emplace(pTargetNode, kCluster);

            for(auto* kParentNode = kTargetNode->GetParent(); (kParentNode && kParentNode->GetParent()); kParentNode = kParentNode->GetParent())
                poseNodeList.emplace(kParentNode);
            poseNodeList.emplace(kTargetNode);
        }
    }

    for(auto edxVert = (unsigned int)ins_newToOldIndexer.size(), idxVert = 0u; idxVert < edxVert; ++idxVert){
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
                    fbx_string msg = FBX_TEXT("failed to link cluster");
                    msg += FBX_TEXT("(errored in \"");
                    msg += strName;
                    msg += FBX_TEXT("\")");
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                    return false;
                }

                kCluster = f->second;
            }

            kCluster->AddControlPointIndex(idxVert, iCluster.second);
        }
    }

    if(kMesh->AddDeformer(kSkin) < 0){
        fbx_string msg = FBX_TEXT("an error occurred while adding deformer");
        msg += FBX_TEXT("(errored in \"");
        msg += strName;
        msg += FBX_TEXT("\")");
        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
        return false;
    }

    poseNodeList.emplace(kNode);

    return true;
}
