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
    void allocateNode(FBXNode*& dest, const FBXNode* src){
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

            if(src->Name){
                const auto i = FBXUtilites::Memlen(src->Name);
                dest->Name = FBXAllocate<char>(i + 1);
                dest->Name[i] = 0;
            }

            if(FBXTypeHasMember(srcID, FBXType::FBXType_Bone)){
                auto* dest_c = static_cast<FBXBone*>(dest);
                const auto* src_c = static_cast<const FBXBone*>(src);
            }
            if(FBXTypeHasMember(srcID, FBXType::FBXType_Mesh)){
                auto* dest_c = static_cast<FBXMesh*>(dest);
                const auto* src_c = static_cast<const FBXMesh*>(src);

                dest_c->Indices = src_c->Indices;
                dest_c->Edges = src_c->Edges;
                dest_c->Vertices = src_c->Vertices;

                dest_c->LayeredVertices = src_c->LayeredVertices;
            }
            if(FBXTypeHasMember(srcID, FBXType::FBXType_SkinnedMesh)){
                auto* dest_c = static_cast<FBXSkinnedMesh*>(dest);
                const auto* src_c = static_cast<const FBXSkinnedMesh*>(src);

                dest_c->SkinInfos = src_c->SkinInfos;
                dest_c->SkinDeforms = src_c->SkinDeforms;
            }

            allocateNode(dest->Child, src->Child);
            allocateNode(dest->Sibling, src->Sibling);
        }
    }

    void allocateAnimation(FBXAnimation*& dest, const FBXAnimation* src){
        if(src){
            dest = FBXNew<FBXAnimation>();



            allocateAnimation(dest->Next, src->Next);
        }
    }
};


FBXRoot* FBXAllocateRoot(const FBXRoot* src){
    if(!src)
        return nullptr;

    auto* dest = FBXNew<FBXRoot>();

    __hidden_FBXModule::allocateNode(dest->Nodes, src->Nodes);
    __hidden_FBXModule::allocateAnimation(dest->Animations, src->Animations);

    return dest;
}