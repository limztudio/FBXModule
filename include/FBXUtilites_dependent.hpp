/**
 * @file FBXUtilites_dependent.hpp
 * @date 2020/05/08
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#ifndef _FBXUTILITES_DEPENDENT_HPP_
#define _FBXUTILITES_DEPENDENT_HPP_


#include "FBXAssign.hpp"
#include "FBXUtilites_independent.hpp"

#include "FBXBase.hpp"

#include "FBXRoot.hpp"

#include "FBXNode.hpp"

#include "FBXBone.hpp"

#include "FBXMesh.hpp"
#include "FBXSkinnedMesh.hpp"

#include "FBXAnimation.hpp"


namespace __hidden_FBXModule{
    template<typename T>
    static void allocateNode(T*& dest, const T* src, T* pDestParent = nullptr){
        if(src){
            const auto srcID = src->getID();
            switch(srcID){
            case FBXType::FBXType_Node:
                dest = FBXNew<FBXNode>();
                break;

            case FBXType::FBXType_Bone:
                dest = FBXNew<FBXBone>();
                break;

            case FBXType::FBXType_Mesh:
                dest = FBXNew<FBXMesh>();
                break;

            case FBXType::FBXType_SkinnedMesh:
                dest = FBXNew<FBXSkinnedMesh>();
                break;

            default:
                return;
            }

            dest->Name = src->Name;
            dest->TransformMatrix = src->TransformMatrix;

            if(FBXTypeHasMember(srcID, FBXType::FBXType_Bone)){
                auto* dest_c = static_cast<FBXBone*>(dest);
                const auto* src_c = static_cast<const FBXBone*>(src);

                dest_c->Size = src_c->Size;
                dest_c->Length = src_c->Length;
            }
            if(FBXTypeHasMember(srcID, FBXType::FBXType_Mesh)){
                auto* dest_c = static_cast<FBXMesh*>(dest);
                const auto* src_c = static_cast<const FBXMesh*>(src);

                dest_c->Materials = src_c->Materials;
                dest_c->Attributes = src_c->Attributes;
                dest_c->Indices = src_c->Indices;
                dest_c->Vertices = src_c->Vertices;

                dest_c->LayeredElements = src_c->LayeredElements;
            }
            if(FBXTypeHasMember(srcID, FBXType::FBXType_SkinnedMesh)){
                auto* dest_c = static_cast<FBXSkinnedMesh*>(dest);
                const auto* src_c = static_cast<const FBXSkinnedMesh*>(src);

                dest_c->BoneCombinations = src_c->BoneCombinations;
                dest_c->SkinInfos = src_c->SkinInfos;
                dest_c->SkinDeforms = src_c->SkinDeforms;
            }

            dest->Parent = pDestParent;
            allocateNode<T>(dest->Child, src->Child, dest);
            allocateNode<T>(dest->Sibling, src->Sibling, pDestParent);
        }
    }
};


template<typename T>
static void FBXCopyRoot(T** pDest, const T* pSrc){
    if(!pSrc)
        return;

    const auto* src = reinterpret_cast<const FBXRoot*>(pSrc);
    auto* dest = FBXNew<FBXRoot>();

    __hidden_FBXModule::allocateNode(dest->Nodes, src->Nodes);
    dest->Materials = src->Materials;
    dest->Animations = src->Animations;

    __hidden_FBXModule_RebindRoot(dest, src);

    (*pDest) = dest;
}

/**
 * @brief Collapse bone node of skinned mesh.
 * @param pRootNode FBX root node. Type must be set to FBXRoot.
 * @param pSkinnedMesh Skinned mesh needs to be collapsed. Type must be set to FBXSkinnedMesh.
 * @param pOldNodes Old bone list of skinned mesh. Type must be set to FBXNode.
 * @param pNewNodes Newer bone list of skinned mesh. Type must be set to FBXNode.
 * @param nodeCount Node count of bone list. Old bone list and newer bone list must be same.
 * @return Return true if successfully collapsed, and false otherwise.
 */
template<typename ROOT, typename SKINNED_MESH, typename NODE>
static bool FBXCollapseBone(ROOT* pFBXRoot, SKINNED_MESH** pSkinnedMesh, const NODE** pOldNodes, const NODE** pNewNodes, unsigned long nodeCount){
    if((*pSkinnedMesh)->getID() != FBXType::FBXType_SkinnedMesh)
        return false;

    decltype(nodeCount) sameCounter = 0u;
    for(decltype(nodeCount) idxNode = 0u; idxNode < nodeCount; ++idxNode){
        if(pOldNodes[idxNode] == pNewNodes[idxNode])
            ++sameCounter;
    }

    if(sameCounter == nodeCount)
        return true;

    FBXSkinnedMesh* pInnerMesh = nullptr;
    if(!__hidden_FBXModule_CollapseMesh(reinterpret_cast<void**>(&pInnerMesh), *pSkinnedMesh, reinterpret_cast<const void**>(pOldNodes), reinterpret_cast<const void**>(pNewNodes), nodeCount))
        return false;

    auto* pNewMesh = FBXNew<FBXSkinnedMesh>();

    {
        pNewMesh->Parent = (*pSkinnedMesh)->Parent;
        pNewMesh->Child = (*pSkinnedMesh)->Child;
        pNewMesh->Sibling = (*pSkinnedMesh)->Sibling;
    }

    FBXIterateNode(pFBXRoot->Nodes, [&pSkinnedMesh, &pNewMesh](FBXNode* pNode){
        if(pNode->Parent == (*pSkinnedMesh))
            pNode->Parent = pNewMesh;
        if(pNode->Sibling == (*pSkinnedMesh))
            pNode->Sibling = pNewMesh;
    });
    for(FBX_SIZE edxAnim = pFBXRoot->Animations.Length, idxAnim = 0u; idxAnim < edxAnim; ++idxAnim){
        auto& cAnim = pFBXRoot->Animations.Values[idxAnim];

        for(FBX_SIZE edxNode = cAnim.AnimationNodes.Length, idxNode = 0u; idxNode < edxNode; ++idxNode){
            auto& cNode = cAnim.AnimationNodes.Values[idxNode];

            if(cNode.BindNode == (*pSkinnedMesh))
                cNode.BindNode = pNewMesh;
        }
    }

    {
        pNewMesh->Name = pInnerMesh->Name;
        pNewMesh->TransformMatrix = pInnerMesh->TransformMatrix;
    }
    {
        pNewMesh->Materials = pInnerMesh->Materials;
        pNewMesh->Materials = pInnerMesh->Materials;
        pNewMesh->Attributes = pInnerMesh->Attributes;
        pNewMesh->Indices = pInnerMesh->Indices;
        pNewMesh->Vertices = pInnerMesh->Vertices;

        pNewMesh->LayeredElements = pInnerMesh->LayeredElements;
    }
    {
        pNewMesh->BoneCombinations = pInnerMesh->BoneCombinations;
        pNewMesh->SkinInfos = pInnerMesh->SkinInfos;
        pNewMesh->SkinDeforms = pInnerMesh->SkinDeforms;

        for(auto* p0 = pNewMesh->BoneCombinations.Values; FBX_PTRDIFFU(p0 - pNewMesh->BoneCombinations.Values) < pNewMesh->BoneCombinations.Length; ++p0){
            for(auto* p1 = p0->Values; FBX_PTRDIFFU(p1 - p0->Values) < p0->Length; ++p1){
                if((*p1) == pInnerMesh)
                    (*p1) = pNewMesh;
            }
        }
        for(auto* p0 = pNewMesh->SkinInfos.Values; FBX_PTRDIFFU(p0 - pNewMesh->SkinInfos.Values) < pNewMesh->SkinInfos.Length; ++p0){
            for(auto* p1 = p0->Values; FBX_PTRDIFFU(p1 - p0->Values) < p0->Length; ++p1){
                if(p1->BindNode == pInnerMesh)
                    p1->BindNode = pNewMesh;
            }
        }
        for(auto* p = pNewMesh->SkinDeforms.Values; FBX_PTRDIFFU(p - pNewMesh->SkinDeforms.Values) < pNewMesh->SkinDeforms.Length; ++p){
            if(p->TargetNode == pInnerMesh)
                p->TargetNode = pNewMesh;
        }
    }

    __hidden_FBXModule_DeleteInnerObject(pInnerMesh);

    {
        (*pSkinnedMesh)->Parent = nullptr;
        (*pSkinnedMesh)->Child = nullptr;
        (*pSkinnedMesh)->Sibling = nullptr;
    }
    FBXDelete(*pSkinnedMesh);

    (*pSkinnedMesh) = pNewMesh;
    return true;
}


#endif // _FBXUTILITES_DEPENDENT_HPP_
