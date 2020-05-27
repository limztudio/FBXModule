/**
* @file FBXAllocate.hpp
* @date 2020/05/08
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXAssign.hpp"
#include "FBXUtilites.hpp"

#include "FBXBase.hpp"

#include "FBXRoot.hpp"

#include "FBXNode.hpp"

#include "FBXBone.hpp"

#include "FBXMesh.hpp"
#include "FBXSkinnedMesh.hpp"

#include "FBXAnimation.hpp"


namespace __hidden_FBXModule{
    template<typename T>
    static void allocateNode(T*& dest, const T* src, T* pDestarent = nullptr){
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

            dest->Parent = pDestarent;
            allocateNode<T>(dest->Child, src->Child, dest);
            allocateNode<T>(dest->Sibling, src->Sibling, dest);
        }
    }
};


template<typename T>
static void FBXAllocateRoot(T** pDest, const T* pSrc){
    if(!pSrc)
        return;

    auto* src = reinterpret_cast<const FBXRoot*>(pSrc);
    auto* dest = FBXNew<FBXRoot>();

    __hidden_FBXModule::allocateNode(dest->Nodes, src->Nodes);

    dest->Materials = src->Materials;
    dest->Animations = src->Animations;

    (*pDest) = dest;
}
