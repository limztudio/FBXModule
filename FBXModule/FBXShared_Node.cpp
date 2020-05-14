/**
* @file FBXShared_Node.cpp
* @date 2019/04/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <eastl/unordered_map.h>

#include <FBXAssign.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


using namespace fbxsdk;


#define _ERROR_INSIDE_ADD_MESH -1
#define _ERROR_INDSID_BIND_SKIN -2


eastl::unordered_map<FbxNode*, FBXNode*> shr_fbxNodeToExportNode;
eastl::unordered_map<const FBXNode*, FbxNode*> shr_ImportedNodeToFbxNode;


struct _NodeData_wrapper{
    FBXNode* ExportNode;
    NodeData NodeData;
};


static inline bool ins_addBoneNode(
    FbxManager* kSDKManager,
    FbxNode* kNode,
    NodeData* pNodeData,
    FBXNode*& pNode
)
{
    auto* kBone = (FbxSkeleton*)kNode->GetNodeAttribute();
    if(!kBone)
        return false;

    auto* pBone = FBXNew<FBXBone>();
    pNode = pBone;

    return true;
}
static inline bool ins_addMeshNode(
    FbxManager* kSDKManager,
    FbxNode* kNode,
    NodeData* pNodeData,
    FBXNode*& pNode
)
{
    auto* kMesh = (FbxMesh*)kNode->GetNodeAttribute();
    if(!kMesh)
        return false;

    if(!kMesh->GetControlPointsCount())
        return false;

    {
        ControlPointRemap controlPointRemap;
        if(!SHRLoadMeshFromNode(controlPointRemap, kNode, pNodeData))
            throw _ERROR_INSIDE_ADD_MESH;

        SHROptimizeMesh(controlPointRemap, pNodeData);

        if(!SHRLoadSkinFromNode(controlPointRemap, kNode, pNodeData))
            throw _ERROR_INSIDE_ADD_MESH;
    }

    {
        if(pNodeData->bufSkinData.empty())
            pNode = FBXNew<FBXMesh>();
        else
            pNode = FBXNew<FBXSkinnedMesh>();
    }

    {
        auto* pMesh = static_cast<FBXMesh*>(pNode);

        pMesh->Indices.Assign(pNodeData->bufIndices.size());
        for(size_t idxInd = 0; idxInd < pMesh->Indices.Length; ++idxInd){
            auto& iInd = pMesh->Indices.Values[idxInd];

            CopyArrayData(iInd.Values, pNodeData->bufIndices[idxInd].raw);
        }

        pMesh->Vertices.Assign(pNodeData->bufPositions.size());
        for(size_t idxVert = 0; idxVert < pMesh->Vertices.Length; ++idxVert){
            auto& iVert = pMesh->Vertices.Values[idxVert];

            CopyArrayData(iVert.Values, pNodeData->bufPositions[idxVert].mData);
        }

        pMesh->LayeredVertices.Assign(pNodeData->bufLayers.size());
        for(size_t idxLayer = 0; idxLayer < pMesh->LayeredVertices.Length; ++idxLayer){
            auto& iLayer = pMesh->LayeredVertices.Values[idxLayer];
            const auto& nodeLayer = pNodeData->bufLayers[idxLayer];

            {
                auto& iObject = iLayer.Material;
                const auto& nodeObject = nodeLayer.materials;

                iObject.Assign(nodeObject.size());
                CopyArrayData(iObject.Values, nodeObject.data(), iObject.Length);
            }

            {
                auto& iObject = iLayer.Color;
                const auto& nodeObject = nodeLayer.colors;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }

            {
                auto& iObject = iLayer.Normal;
                const auto& nodeObject = nodeLayer.normals;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }

            {
                auto& iObject = iLayer.Binormal;
                const auto& nodeObject = nodeLayer.binormals;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }

            {
                auto& iObject = iLayer.Tangent;
                const auto& nodeObject = nodeLayer.tangents;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }

            {
                auto& iObject = iLayer.Texcoord;
                const auto& nodeObject = nodeLayer.texcoords.table;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }
        }
    }

    if(!pNodeData->bufSkinData.empty()){
        auto* pMesh = static_cast<FBXSkinnedMesh*>(pNode);

        pMesh->SkinInfos.Assign(pNodeData->bufSkinData.size());
        for(size_t idxSkin = 0; idxSkin < pMesh->SkinInfos.Length; ++idxSkin){
            auto& iSkin = pMesh->SkinInfos.Values[idxSkin];
            const auto& nodeSkin = pNodeData->bufSkinData[idxSkin];

            iSkin.Assign(nodeSkin.size());
            for(size_t idxCluster = 0; idxCluster < iSkin.Length; ++idxCluster){
                auto& iCluster = iSkin.Values[idxCluster];
                const auto& nodeCluster = nodeSkin[idxCluster];

                auto* kBindNode = nodeCluster.cluster->GetLink();

                iCluster.BindNode = reinterpret_cast<decltype(iCluster.BindNode)>(kBindNode);
                iCluster.Weight = static_cast<decltype(iCluster.Weight)>(nodeCluster.weight);
            }
        }

        pMesh->SkinDeforms.Assign(pNodeData->mapBoneDeformMatrices.size());

        auto* iDeform = pMesh->SkinDeforms.Values;
        for(const auto& nodeDeform : pNodeData->mapBoneDeformMatrices){
            auto* kBindNode = nodeDeform.first->GetLink();

            iDeform->TargetNode = reinterpret_cast<decltype(iDeform->TargetNode)>(kBindNode);

            const auto& matrix = nodeDeform.second;
            CopyArrayData(iDeform->DeformMatrix.Values, (const double*)matrix);

            ++iDeform;
        }
    }

    return true;
}

static void ins_addNodeRecursive(
    FbxManager* kSDKManager,
    FbxPose* kPose,
    FbxNode* kNode,
    FBXNode*& pNode
)
{
    if(!kNode)
        return;

    {
        NodeData genNodeData;

        {
            genNodeData.strName = kNode->GetName();
        }

        {
            auto& kOrientedPos = genNodeData.kTransformMatrix;
            kOrientedPos = GetLocalTransform(kNode);
        }

        {
            auto* kNodeAttr = kNode->GetNodeAttribute();
            auto kAttrType = FbxNodeAttribute::eNull;

            if(kNodeAttr)
                kAttrType = kNodeAttr->GetAttributeType();

            switch(kAttrType){
            case FbxNodeAttribute::eSkeleton:
            {
                if(!ins_addBoneNode(
                    kSDKManager,
                    kNode,
                    &genNodeData,
                    pNode
                ))
                    goto _ADD_NODE_UNKNOWN_TYPE;
            }
            break;

            case FbxNodeAttribute::eMesh:
            {
                if(!ins_addMeshNode(
                    kSDKManager,
                    kNode,
                    &genNodeData,
                    pNode
                ))
                    goto _ADD_NODE_UNKNOWN_TYPE;
            }
            break;

            default:
            {
                goto _ADD_NODE_UNKNOWN_TYPE;
            }
            break;
            }
            goto _ADD_NODE_AFTER_ALLOCATE;

        _ADD_NODE_UNKNOWN_TYPE:
            pNode = FBXNew<FBXNode>();
        }

    _ADD_NODE_AFTER_ALLOCATE:
        {
            pNode->Name = CopyString(genNodeData.strName);

            const auto& matrix = genNodeData.kTransformMatrix;
            CopyArrayData(pNode->TransformMatrix.Values, (const double*)matrix);
        }

        {
            shr_fbxNodeToExportNode.emplace(kNode, pNode);
        }
    }

    {
        const auto e = kNode->GetChildCount();
        if(e){
            {
                ins_addNodeRecursive(
                    kSDKManager,
                    kPose,
                    kNode->GetChild(0),
                    pNode->Child
                );
            }

            auto** pCurChildNode = &pNode->Child->Sibling;
            for(auto i = decltype(e){ 1 }; i < e; ++i, pCurChildNode = &(*pCurChildNode)->Sibling)
                ins_addNodeRecursive(
                    kSDKManager,
                    kPose,
                    kNode->GetChild(i),
                    *pCurChildNode
                );
        }
    }
}

static void ins_bindSkinningInfoRecursive(FBXNode* pNode){
    if(pNode->getID() == FBXType::FBXType_SkinnedMesh){
        auto* pMesh = static_cast<FBXSkinnedMesh*>(pNode);

        for(auto* iVert = pMesh->SkinInfos.Values; size_t(iVert - pMesh->SkinInfos.Values) < pMesh->SkinInfos.Length; ++iVert){
            for(auto* iWeight = iVert->Values; size_t(iWeight - iVert->Values) < iVert->Length; ++iWeight){
                auto* kBindNode = reinterpret_cast<FbxNode*>(iWeight->BindNode);

                auto f = shr_fbxNodeToExportNode.find(kBindNode);
                if(f == shr_fbxNodeToExportNode.cend())
                    throw _ERROR_INDSID_BIND_SKIN;

                iWeight->BindNode = f->second;
            }
        }

        for(auto* iDeform = pMesh->SkinDeforms.Values; size_t(iDeform - pMesh->SkinDeforms.Values) < pMesh->SkinDeforms.Length; ++iDeform){
            auto* kBindNode = reinterpret_cast<FbxNode*>(iDeform->TargetNode);

            auto f = shr_fbxNodeToExportNode.find(kBindNode);
            if(f == shr_fbxNodeToExportNode.cend())
                throw _ERROR_INDSID_BIND_SKIN;

            iDeform->TargetNode = f->second;
        }
    }

    {
        if(pNode->Sibling)
            ins_bindSkinningInfoRecursive(pNode->Sibling);

        if(pNode->Child)
            ins_bindSkinningInfoRecursive(pNode->Child);
    }
}


bool SHRGenerateNodeTree(FbxManager* kSDKManager, FbxScene* kScene){
    static const char __name_of_this_func[] = "SHRGenerateNodeTree(FbxManager*, FbxScene*)";


    if(shr_root->Nodes){
        SHRPushErrorMessage("scene must be destroyed before create", __name_of_this_func);
        return false;
    }

    {
        auto* kRootNode = kScene->GetRootNode();

        FbxPose* kPose = nullptr;
        int poseIndex = -1;

        if(poseIndex >= 0)
            kPose = kScene->GetPose(poseIndex);

        try{
            shr_fbxNodeToExportNode.clear();

            ins_addNodeRecursive(
                kSDKManager,
                kPose,
                kRootNode,
                shr_root->Nodes
            );

            ins_bindSkinningInfoRecursive(shr_root->Nodes);
        }
        catch(int ret){
            switch(ret){
            case _ERROR_INSIDE_ADD_MESH:
                break;

            case _ERROR_INDSID_BIND_SKIN:
                SHRPushErrorMessage("an error occurred while binding skinning info", __name_of_this_func);
                break;
            }
            return false;
        }
    }

    return true;
}

bool SHRStoreNodes(FbxManager* kSDKManager, FbxScene* kScene, const FBXNode* pRootNode){
    static const char __name_of_this_func[] = "SHRStoreNodes(FbxManager*, FbxScene*, const FBXNode*)";


    shr_ImportedNodeToFbxNode.clear();

    if(pRootNode){
        if(pRootNode->Child){
            auto* kRootNode = kScene->GetRootNode();

            auto* kNewNode = SHRStoreNode(kSDKManager, kRootNode, pRootNode->Child);
            if(!kNewNode)
                return false;

            if(!kRootNode->AddChild(kNewNode)){
                SHRPushErrorMessage("an error occurred while adding child node", __name_of_this_func);
                return false;
            }

            shr_ImportedNodeToFbxNode.emplace(pRootNode, kRootNode);
        }
    }

    for(auto i = shr_ImportedNodeToFbxNode.begin(), e = shr_ImportedNodeToFbxNode.end(); i != e; ++i){
        switch(i->first->getID()){
        case FBXType::FBXType_Bone:
            if(!SHRInitBoneNode(kSDKManager, static_cast<const FBXBone*>(i->first), i->second))
                return false;
            break;

        case FBXType::FBXType_Mesh:
            if(!SHRInitMeshNode(kSDKManager, static_cast<const FBXMesh*>(i->first), i->second))
                return false;
            break;

        case FBXType::FBXType_SkinnedMesh:
            if(!SHRInitSkinnedMeshNode(kSDKManager, static_cast<const FBXSkinnedMesh*>(i->first), i->second))
                return false;
            break;
        }
    }

    return true;
}
FbxNode* SHRStoreNode(FbxManager* kSDKManager, FbxNode* kParentNode, const FBXNode* pNode){
    static const char __name_of_this_func[] = "SHRStoreNode(FbxManager*, FbxNode*, const FBXNode*)";


    if(pNode){
        auto* kNode = FbxNode::Create(kSDKManager, pNode->Name);
        if(!kNode){
            SHRPushErrorMessage("failed to create FbxNode", __name_of_this_func);
            return nullptr;
        }

        switch(pNode->getID()){
        case FBXType::FBXType_Bone:
        {
            auto* kSkeleton = FbxSkeleton::Create(kSDKManager, pNode->Name);
            if(!kSkeleton){
                SHRPushErrorMessage("failed to create FbxSkeleton", __name_of_this_func);
                return nullptr;
            }

            kNode->SetNodeAttribute(kSkeleton);
            shr_ImportedNodeToFbxNode.emplace(pNode, kNode);
            break;
        }

        case FBXType::FBXType_Mesh:
        {
            auto* kMesh = FbxMesh::Create(kSDKManager, pNode->Name);
            if(!kMesh){
                SHRPushErrorMessage("failed to create FbxMesh", __name_of_this_func);
                return nullptr;
            }

            kNode->SetNodeAttribute(kMesh);
            shr_ImportedNodeToFbxNode.emplace(pNode, kNode);
            break;
        }

        case FBXType::FBXType_SkinnedMesh:
        {
            auto* kMesh = FbxMesh::Create(kSDKManager, pNode->Name);
            if(!kMesh){
                SHRPushErrorMessage("failed to create FbxMesh", __name_of_this_func);
                return nullptr;
            }

            auto* kSkin = FbxSkin::Create(kSDKManager, pNode->Name);
            if(!kSkin){
                SHRPushErrorMessage("failed to create FbxSkin", __name_of_this_func);
                return nullptr;
            }

            if(kMesh->AddDeformer(kSkin) < 0){
                SHRPushErrorMessage("an error occurred while adding deformer", __name_of_this_func);
                return nullptr;
            }

            kNode->SetNodeAttribute(kMesh);
            shr_ImportedNodeToFbxNode.emplace(pNode, kNode);
            break;
        }
        }

        {
            FbxAMatrix matTransform;
            CopyArrayData<_countof(pNode->TransformMatrix.Values)>((double*)matTransform, pNode->TransformMatrix.Values);

            kNode->LclTranslation.Set(matTransform.GetT());
            kNode->LclRotation.Set(matTransform.GetR());
            kNode->LclScaling.Set(matTransform.GetS());
        }

        if(pNode->Child){
            auto* kNewNode = SHRStoreNode(kSDKManager, kNode, pNode->Child);
            if(!kNewNode)
                return nullptr;

            if(!kNode->AddChild(kNewNode)){
                SHRPushErrorMessage("an error occurred while adding child node", __name_of_this_func);
                return nullptr;
            }
        }
        if(pNode->Sibling){
            auto* kNewNode = SHRStoreNode(kSDKManager, kParentNode, pNode->Sibling);
            if(!kNewNode)
                return nullptr;

            if(!kParentNode->AddChild(kNewNode)){
                SHRPushErrorMessage("an error occurred while adding sibling node", __name_of_this_func);
                return nullptr;
            }
        }

        return kNode;
    }

    return nullptr;
}
