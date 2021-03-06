﻿/**
 * @file FBXShared_Node.cpp
 * @date 2019/04/11
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#include "stdafx.h"

#include <FBXAssign.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


using namespace fbxsdk;


#define _ERROR_INSIDE_ADD_MESH -1
#define _ERROR_INDSID_BIND_SKIN -2


struct _NodeData_wrapper{
    FBXNode* ExportNode;
    NodeData NodeData;
};


static ControlPointMergeMap ins_controlPointMergeMap;
static fbx_unordered_set<FbxNode*, PointerHasher<FbxNode*>> ins_linkedNodes;


FbxNodeToExportNode shr_fbxNodeToExportNode;
ImportNodeToFbxNode shr_importNodeToFbxNode;
PoseNodeList shr_poseNodeList;


static inline void ins_collectUnlinkedNode(FbxNode* kNewNode, FBXNode* pRootNode, FbxNodeToExportNode& fbxNodeToExportNode){
    if(!kNewNode)
        return;

    if(fbxNodeToExportNode.find(kNewNode) != fbxNodeToExportNode.cend())
        return;

    auto* kParentNode = kNewNode->GetParent();
    ins_collectUnlinkedNode(kParentNode, pRootNode, fbxNodeToExportNode);

    auto* pParentNode = pRootNode;
    {
        auto f = fbxNodeToExportNode.find(kParentNode);
        if(f != fbxNodeToExportNode.cend())
            pParentNode = f->second;
    }

    auto* pNewNode = FBXNew<FBXNode>();
    {
        CopyString(pNewNode->Name, ConvertString<FBX_CHAR>(kNewNode->GetName()));
        if(kParentNode)
            CopyArrayData(pNewNode->TransformMatrix.Values, (const double*)GetLocalTransform(kNewNode));
        else
            CopyArrayData(pNewNode->TransformMatrix.Values, (const double*)GetGlobalTransform(kNewNode));
    }

    pNewNode->Parent = pParentNode;
    FBXFindLastAddible(pParentNode->Child) = pNewNode;

    fbxNodeToExportNode.emplace(kNewNode, pNewNode);
}
static inline void ins_collectUnlinkedNodes(FBXNode* pRootNode, FbxNodeToExportNode& fbxNodeToExportNode){
    for(auto* kNode : ins_linkedNodes)
        ins_collectUnlinkedNode(kNode, pRootNode, fbxNodeToExportNode);
}

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

    if(!SHRLoadBoneNode(kSDKManager, kNode, pBone))
        return false;

    return true;
}
static inline bool ins_addMeshNode(
    FbxManager* kSDKManager,
    MaterialTable& materialTable,
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
        if(!SHRLoadMeshFromNode(materialTable, controlPointRemap, kNode, pNodeData))
            throw _ERROR_INSIDE_ADD_MESH;

        if(!SHRLoadSkinFromNode(controlPointRemap, kNode, pNodeData))
            throw _ERROR_INSIDE_ADD_MESH;

        SHROptimizeMesh(pNodeData);

        SHRGenerateMeshAttribute(pNodeData);
    }

    {
        if(pNodeData->bufSkinData.empty())
            pNode = FBXNew<FBXMesh>();
        else
            pNode = FBXNew<FBXSkinnedMesh>();
    }

    {
        auto* pMesh = static_cast<FBXMesh*>(pNode);

        pMesh->Attributes.Assign(pNodeData->bufMeshAttribute.size());
        for(size_t idxAttr = 0u; idxAttr < pMesh->Attributes.Length; ++idxAttr){
            const auto& iOldAttr = pNodeData->bufMeshAttribute[idxAttr];
            auto& iNewAttr = pMesh->Attributes.Values[idxAttr];

            iNewAttr.VertexStart = iOldAttr.VertexFirst;
            iNewAttr.IndexStart = iOldAttr.PolygonFirst;

            iNewAttr.VertexCount = 1 + iOldAttr.VertexLast - iOldAttr.VertexFirst;
            iNewAttr.IndexCount = 1 + iOldAttr.PolygonLast - iOldAttr.PolygonFirst;
        }

        pMesh->Indices.Assign(pNodeData->bufIndices.size());
        for(size_t idxInd = 0u; idxInd < pMesh->Indices.Length; ++idxInd){
            auto& iInd = pMesh->Indices.Values[idxInd];

            CopyArrayData(iInd.Values, pNodeData->bufIndices[idxInd].raw);
        }

        pMesh->Vertices.Assign(pNodeData->bufPositions.size());
        for(size_t idxVert = 0u; idxVert < pMesh->Vertices.Length; ++idxVert){
            auto& iVert = pMesh->Vertices.Values[idxVert];

            CopyArrayData(iVert.Values, pNodeData->bufPositions[idxVert].mData);
        }

        pMesh->LayeredElements.Assign(pNodeData->bufLayers.size());
        for(size_t idxLayer = 0u; idxLayer < pMesh->LayeredElements.Length; ++idxLayer){
            auto& iLayer = pMesh->LayeredElements.Values[idxLayer];
            const auto& nodeLayer = pNodeData->bufLayers[idxLayer];

            {
                auto& iObject = iLayer.Material;
                const auto& nodeObject = nodeLayer.materials;

                if(nodeObject.empty())
                    iObject.Assign(0u);
                else{
                    iObject.Assign(pNodeData->bufMeshAttribute.size());
                    for(size_t idxMat = 0u; idxMat < iObject.Length; ++idxMat){
                        const auto idxOldMat = pMesh->Attributes.Values[idxMat].IndexStart;
                        iObject.Values[idxMat] = nodeObject[idxOldMat];
                    }
                }
            }

            {
                auto& iObject = iLayer.Color;
                const auto& nodeObject = nodeLayer.colors;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0u; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }

            {
                auto& iObject = iLayer.Normal;
                const auto& nodeObject = nodeLayer.normals;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0u; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }

            {
                auto& iObject = iLayer.Binormal;
                const auto& nodeObject = nodeLayer.binormals;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0u; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }

            {
                auto& iObject = iLayer.Tangent;
                const auto& nodeObject = nodeLayer.tangents;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0u; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }

            {
                auto& iObject = iLayer.Texcoord;
                const auto& nodeObject = nodeLayer.texcoords.table;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0u; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }
        }

        pMesh->Materials.Assign(pNodeData->bufMaterials.size());
        for(size_t idxMaterial = 0u; idxMaterial < pMesh->Materials.Length; ++idxMaterial){
            auto& iMaterial = pMesh->Materials.Values[idxMaterial];
            const auto& nodeMaterial = pNodeData->bufMaterials[idxMaterial];

            iMaterial = nodeMaterial;
        }
    }

    if(!pNodeData->bufSkinData.empty()){
        auto* pMesh = static_cast<FBXSkinnedMesh*>(pNode);

        pMesh->BoneCombinations.Assign(pNodeData->bufBoneCombination.size());
        for(size_t idxAttr = 0u; idxAttr < pMesh->BoneCombinations.Length; ++idxAttr){
            auto& iAttr = pMesh->BoneCombinations.Values[idxAttr];
            const auto& nodeAttr = pNodeData->bufBoneCombination[idxAttr];

            iAttr.Assign(nodeAttr.size());
            for(size_t idxBC = 0u; idxBC < iAttr.Length; ++idxBC){
                auto*& iCluster = iAttr.Values[idxBC];
                auto* nodeCluster = nodeAttr[idxBC];

                auto* kBindNode = nodeCluster->GetLink();
                ins_linkedNodes.emplace(kBindNode);

                iCluster = reinterpret_cast<decltype(iCluster)>(kBindNode);
            }
        }

        pMesh->SkinInfos.Assign(pNodeData->bufSkinData.size());
        for(size_t idxSkin = 0u; idxSkin < pMesh->SkinInfos.Length; ++idxSkin){
            auto& iSkin = pMesh->SkinInfos.Values[idxSkin];
            const auto& nodeSkin = pNodeData->bufSkinData[idxSkin];

            iSkin.Assign(nodeSkin.size());
            for(size_t idxCluster = 0u; idxCluster < iSkin.Length; ++idxCluster){
                auto& iCluster = iSkin.Values[idxCluster];
                const auto& nodeCluster = nodeSkin[idxCluster];

                auto* kBindNode = nodeCluster.cluster->GetLink();
                ins_linkedNodes.emplace(kBindNode);

                iCluster.BindNode = reinterpret_cast<decltype(iCluster.BindNode)>(kBindNode);
                iCluster.Weight = static_cast<decltype(iCluster.Weight)>(nodeCluster.weight);
            }
        }

        pMesh->SkinDeforms.Assign(pNodeData->mapBoneDeformMatrices.size());
        auto* iDeform = pMesh->SkinDeforms.Values;
        for(const auto& nodeDeform : pNodeData->mapBoneDeformMatrices){
            auto* kBindNode = nodeDeform.first->GetLink();
            ins_linkedNodes.emplace(kBindNode);

            iDeform->TargetNode = reinterpret_cast<decltype(iDeform->TargetNode)>(kBindNode);

            CopyArrayData(iDeform->TransformMatrix.Values, (const double*)nodeDeform.second.first);
            CopyArrayData(iDeform->LinkMatrix.Values, (const double*)nodeDeform.second.second);

            ++iDeform;
        }
    }

    return true;
}

static void ins_addNodeRecursive(
    FbxManager* kSDKManager,
    MaterialTable& materialTable,
    FbxNodeToExportNode& fbxNodeToExportNode,
    FbxPose* kPose,
    FbxNode* kNode,
    FBXNode*& pNode,
    FBXNode* pParentNode = nullptr
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
                    materialTable,
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
            CopyString(pNode->Name, ConvertString<FBX_CHAR>(genNodeData.strName));

            const auto& matrix = genNodeData.kTransformMatrix;
            CopyArrayData(pNode->TransformMatrix.Values, (const double*)matrix);
        }

        {
            fbxNodeToExportNode.emplace(kNode, pNode);
        }
    }

    {
        pNode->Parent = pParentNode;

        const auto e = kNode->GetChildCount();
        if(e){
            {
                ins_addNodeRecursive(
                    kSDKManager,
                    materialTable,
                    fbxNodeToExportNode,
                    kPose,
                    kNode->GetChild(0),
                    pNode->Child,
                    pNode
                );
            }

            auto** pCurChildNode = &pNode->Child->Sibling;
            for(auto i = decltype(e){ 1 }; i < e; ++i, pCurChildNode = &(*pCurChildNode)->Sibling){
                ins_addNodeRecursive(
                    kSDKManager,
                    materialTable,
                    fbxNodeToExportNode,
                    kPose,
                    kNode->GetChild(i),
                    *pCurChildNode,
                    pNode
                );
            }
        }
    }
}

static void ins_bindSkinningInfoRecursive(const FbxNodeToExportNode& fbxNodeToExportNode, FBXNode* pNode){
    if(pNode->getID() == FBXType::FBXType_SkinnedMesh){
        auto* pMesh = static_cast<FBXSkinnedMesh*>(pNode);

        for(auto* iAttr = pMesh->BoneCombinations.Values; FBX_PTRDIFFU(iAttr - pMesh->BoneCombinations.Values) < pMesh->BoneCombinations.Length; ++iAttr){
            for(auto** iCluster = iAttr->Values; FBX_PTRDIFFU(iCluster - iAttr->Values) < iAttr->Length; ++iCluster){
                auto* kBindNode = reinterpret_cast<FbxNode*>(*iCluster);

                auto f = fbxNodeToExportNode.find(kBindNode);
                if(f == fbxNodeToExportNode.cend())
                    throw _ERROR_INDSID_BIND_SKIN;

                (*iCluster) = f->second;
            }
        }

        for(auto* iVert = pMesh->SkinInfos.Values; FBX_PTRDIFFU(iVert - pMesh->SkinInfos.Values) < pMesh->SkinInfos.Length; ++iVert){
            for(auto* iWeight = iVert->Values; FBX_PTRDIFFU(iWeight - iVert->Values) < iVert->Length; ++iWeight){
                auto* kBindNode = reinterpret_cast<FbxNode*>(iWeight->BindNode);

                auto f = fbxNodeToExportNode.find(kBindNode);
                if(f == fbxNodeToExportNode.cend())
                    throw _ERROR_INDSID_BIND_SKIN;

                iWeight->BindNode = f->second;
            }
        }

        for(auto* iDeform = pMesh->SkinDeforms.Values; FBX_PTRDIFFU(iDeform - pMesh->SkinDeforms.Values) < pMesh->SkinDeforms.Length; ++iDeform){
            auto* kBindNode = reinterpret_cast<FbxNode*>(iDeform->TargetNode);

            auto f = fbxNodeToExportNode.find(kBindNode);
            if(f == fbxNodeToExportNode.cend())
                throw _ERROR_INDSID_BIND_SKIN;

            iDeform->TargetNode = f->second;
        }
    }

    {
        if(pNode->Sibling)
            ins_bindSkinningInfoRecursive(fbxNodeToExportNode, pNode->Sibling);

        if(pNode->Child)
            ins_bindSkinningInfoRecursive(fbxNodeToExportNode, pNode->Child);
    }
}


bool SHRGenerateNodeTree(FbxManager* kSDKManager, FbxScene* kScene, MaterialTable& materialTable, FbxNodeToExportNode& fbxNodeToExportNode, FBXNode** pRootNode){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRGenerateNodeTree(FbxManager*, FbxScene*, MaterialTable&, FbxNodeToExportNode&, FBXNode**)");


    if(*pRootNode){
        SHRPushErrorMessage(FBX_TEXT("scene must be destroyed before create"), __name_of_this_func);
        return false;
    }

    {
        for(auto e = kScene->GetMaterialCount(), i = 0; i < e; ++i){
            auto* kMaterial = kScene->GetMaterial(i);

            if(kMaterial)
                materialTable.emplace(kMaterial);
        }
    }

    {
        auto* kRootNode = kScene->GetRootNode();

        FbxPose* kPose = nullptr;
        int poseIndex = -1;

        if(poseIndex >= 0)
            kPose = kScene->GetPose(poseIndex);

        ins_linkedNodes.clear();

        try{
            ins_addNodeRecursive(
                kSDKManager,
                materialTable,
                fbxNodeToExportNode,
                kPose,
                kRootNode,
                *pRootNode
            );

            const auto nodeCount = fbxNodeToExportNode.size();
            ins_collectUnlinkedNodes(*pRootNode, fbxNodeToExportNode);
            const auto unlinkedNodeCount = fbxNodeToExportNode.size() - nodeCount;

            if(unlinkedNodeCount > 0)
                SHRPushWarningMessage(ToString<FBX_CHAR>(unlinkedNodeCount) + FBX_TEXT(" of unlinked(from root node) node(s) found.\n those nodes won't be loaded properly."), __name_of_this_func);

            ins_bindSkinningInfoRecursive(fbxNodeToExportNode, *pRootNode);
        }
        catch(int ret){
            switch(ret){
            case _ERROR_INSIDE_ADD_MESH:
                break;

            case _ERROR_INDSID_BIND_SKIN:
                SHRPushErrorMessage(FBX_TEXT("an error occurred while binding skinning info"), __name_of_this_func);
                break;
            }
            return false;
        }
    }

    {
        const auto oldSize = materialTable.getTable().size();

        fbx_set<unsigned long> matRef;
        FBXIterateNode((*pRootNode)->Child, [&matRef](const FBXNode* pNode){
            if(FBXTypeHasMember(pNode->getID(), FBXType::FBXType_Mesh)){
                const auto* pMesh = static_cast<const FBXMesh*>(pNode);
                for(const auto* pMat = pMesh->Materials.Values; FBX_PTRDIFFU(pMat - pMesh->Materials.Values) < pMesh->Materials.Length; ++pMat)
                    matRef.emplace(*pMat);
            }
        });

        if(matRef.size() != oldSize){
            fbx_vector<FbxSurfaceMaterial*> matOldData(materialTable.getTable());
            fbx_vector<unsigned long> matIndexer(oldSize, (unsigned long)(-1));

            materialTable.clear();
            for(const auto& i : matRef)
                matIndexer[i] = (unsigned long)materialTable.emplace(matOldData[i]);

            FBXIterateNode((*pRootNode)->Child, [&matIndexer](FBXNode* pNode){
                if(FBXTypeHasMember(pNode->getID(), FBXType::FBXType_Mesh)){
                    auto* pMesh = static_cast<FBXMesh*>(pNode);
                    for(auto* pMat = pMesh->Materials.Values; FBX_PTRDIFFU(pMat - pMesh->Materials.Values) < pMesh->Materials.Length; ++pMat)
                        *pMat = matIndexer[*pMat];
                }
            });
        }
    }

    return true;
}

FbxNode* SHRStoreNode(FbxManager* kSDKManager, ImportNodeToFbxNode& importNodeToFbxNode, FbxNode* kParentNode, const FBXNode* pNode){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRStoreNode(FbxManager*, ImportNodeToFbxNode&, FbxNode*, const FBXNode*)");


    if(pNode){
        const fbx_string strName = pNode->Name.Values;

        auto* kNode = FbxNode::Create(kSDKManager, ConvertString<char>(strName).c_str());
        if(!kNode){
            fbx_string msg = FBX_TEXT("failed to create FbxNode");
            msg += FBX_TEXT("(errored in \"");
            msg += strName;
            msg += FBX_TEXT("\")");
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return nullptr;
        }

        const auto curID = pNode->getID();

        if(FBXTypeHasMember(curID, FBXType::FBXType_Bone)){
            auto* kSkeleton = FbxSkeleton::Create(kSDKManager, "");
            if(!kSkeleton){
                fbx_string msg = FBX_TEXT("failed to create FbxSkeleton");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return nullptr;
            }

            kNode->SetNodeAttribute(kSkeleton);

            if(kNode->GetNodeAttribute() != kSkeleton){
                fbx_string msg = FBX_TEXT("failed to set node attribute");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return nullptr;
            }
        }

        if(FBXTypeHasMember(curID, FBXType::FBXType_Mesh)){
            auto* kMesh = FbxMesh::Create(kSDKManager, "");
            if(!kMesh){
                fbx_string msg = FBX_TEXT("failed to create FbxMesh");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return nullptr;
            }

            kNode->SetNodeAttribute(kMesh);

            if(kNode->GetNodeAttribute() != kMesh){
                fbx_string msg = FBX_TEXT("failed to set node attribute");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return nullptr;
            }
        }

        {
            FbxAMatrix matTransform;
            CopyArrayData<pNode->TransformMatrix.Length>((double*)matTransform, pNode->TransformMatrix.Values);

            kNode->LclTranslation.Set(matTransform.GetT());
            kNode->LclRotation.Set(matTransform.GetR());
            kNode->LclScaling.Set(matTransform.GetS());
        }

        importNodeToFbxNode.emplace(pNode, kNode);

        if(pNode->Child){
            auto* kNewNode = SHRStoreNode(kSDKManager, importNodeToFbxNode, kNode, pNode->Child);
            if(!kNewNode)
                return nullptr;

            if(!kNode->AddChild(kNewNode)){
                fbx_string msg = FBX_TEXT("an error occurred while adding child node");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return nullptr;
            }
        }
        if(pNode->Sibling){
            auto* kNewNode = SHRStoreNode(kSDKManager, importNodeToFbxNode, kParentNode, pNode->Sibling);
            if(!kNewNode)
                return nullptr;

            if(!kParentNode->AddChild(kNewNode)){
                fbx_string msg = FBX_TEXT("an error occurred while adding sibling node");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return nullptr;
            }
        }

        return kNode;
    }

    return nullptr;
}
bool SHRStoreNodes(FbxManager* kSDKManager, FbxScene* kScene, ImportNodeToFbxNode& importNodeToFbxNode, PoseNodeList& poseNodeList, const FBXNode* pRootNode){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRStoreNodes(FbxManager*, FbxScene*, ImportNodeToFbxNode&, PoseNodeList&, const FBXNode*)");


    importNodeToFbxNode.clear();
    poseNodeList.clear();

    {
        size_t iNodeCount = 0u;
        FBXIterateNode(pRootNode, [&iNodeCount](const FBXNode*){ ++iNodeCount; });
        if(!iNodeCount)
            ++iNodeCount;

        importNodeToFbxNode.rehash(iNodeCount << 2);
        poseNodeList.rehash(iNodeCount << 2);
    }

    if(pRootNode){
        // if the first node has no sibling, treat it as root node
        if(!pRootNode->Sibling){
            const fbx_string strName = pRootNode->Name.Values;

            if(pRootNode->Child){
                auto* kRootNode = kScene->GetRootNode();

                auto* kNewNode = SHRStoreNode(kSDKManager, importNodeToFbxNode, kRootNode, pRootNode->Child);
                if(!kNewNode)
                    return false;

                if(!kRootNode->AddChild(kNewNode)){
                    fbx_string msg = FBX_TEXT("an error occurred while adding child node");
                    msg += FBX_TEXT("(errored in \"");
                    msg += strName;
                    msg += FBX_TEXT("\")");
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                    return false;
                }

                importNodeToFbxNode.emplace(pRootNode, kRootNode);
            }
        }
        else{
            auto* kRootNode = kScene->GetRootNode();

            if(!SHRStoreNode(kSDKManager, importNodeToFbxNode, kRootNode, pRootNode))
                return false;
        }
    }

    for(auto& i : importNodeToFbxNode){
        const auto curID = i.first->getID();

        if(FBXTypeHasMember(curID, FBXType::FBXType_Bone)){
            if(!SHRInitBoneNode(kSDKManager, static_cast<const FBXBone*>(i.first), i.second))
                return false;
        }

        ins_controlPointMergeMap.clear();

        if(FBXTypeHasMember(curID, FBXType::FBXType_Mesh)){
            if(!SHRInitMeshNode(kSDKManager, kScene, ins_controlPointMergeMap, static_cast<const FBXMesh*>(i.first), i.second))
                return false;
        }

        if(FBXTypeHasMember(curID, FBXType::FBXType_SkinnedMesh)){
            if(!SHRInitSkinData(kSDKManager, poseNodeList, importNodeToFbxNode, ins_controlPointMergeMap, static_cast<const FBXSkinnedMesh*>(i.first), i.second))
                return false;
        }
    }

    for(auto& i : importNodeToFbxNode){
        const auto curID = i.first->getID();

        const fbx_string strName = i.first->Name.Values;

        auto* kNode = i.second;

        if(FBXTypeHasMember(curID, FBXType::FBXType_Mesh)){
            auto* kNodeAttribute = kNode->GetNodeAttribute();
            if(!kNodeAttribute){
                fbx_string msg = FBX_TEXT("node must have attribute");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            auto* kMesh = static_cast<FbxMesh*>(kNodeAttribute);

            if(kMesh->RemoveBadPolygons() < 0){
                fbx_string msg = FBX_TEXT("failed to remove bad polygons");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            kMesh->BuildMeshEdgeArray();
        }
    }

    return true;
}

bool SHRCreateBindPose(FbxManager* kSDKManager, FbxScene* kScene, const PoseNodeList& poseNodeList){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRCreateBindPose(FbxManager*, FbxScene*, const PoseNodeList&)");


    if(!poseNodeList.empty()){
        auto* kPose = FbxPose::Create(kSDKManager, "");
        if(!kPose){
            SHRPushErrorMessage(FBX_TEXT("an error occurred while creating FbxPose"), __name_of_this_func);
            return false;
        }

        kPose->SetIsBindPose(true);

        for(auto* kNode : poseNodeList){
            const fbx_string strName = ConvertString<FBX_CHAR>(kNode->GetName());

            auto kMat = GetGlobalTransform(kNode);
            if(kPose->Add(kNode, kMat) < 0){
                fbx_string msg = FBX_TEXT("an error occurred while adding pose matrix. cannot find bind node");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }
        }

        kScene->AddPose(kPose);
    }

    return true;
}
