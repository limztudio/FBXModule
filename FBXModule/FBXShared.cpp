/**
* @file FBXShared.cpp
* @date 2020/05/08
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <eastl/unordered_map.h>

#include "FBXUtilites.h"
#include "FBXShared.h"


FBXRoot* shr_root = nullptr;

static eastl::unordered_map<const FBXNode*, FBXNode*> ins_nodeBinder;


void SHRCreateRoot(){
    if(shr_root)
        FBXDelete(shr_root);

    shr_root = FBXNew<FBXRoot>();
}
void SHRDeleteRoot(){
    if(shr_root){
        FBXDelete(shr_root);
        shr_root = nullptr;
    }
}

void SHRNodeBinder(FBXNode* dest, const FBXNode* src){
    static const char __name_of_this_func[] = "SHRNodeBinder(FBXNode*, const FBXNode*)";


    if(dest && src){
        ins_nodeBinder[src] = dest;

        SHRNodeBinder(dest->Child, src->Child);
        SHRNodeBinder(dest->Sibling, src->Sibling);
    }
    else if(dest && (!src))
        SHRPushErrorMessage("destination and source must have value. but source is null", __name_of_this_func);
    else if((!dest) && src)
        SHRPushErrorMessage("destination and source must have value. but destination is null", __name_of_this_func);
}

void SHRCopyRoot(FBXRoot* dest, const FBXRoot* src){
    ins_nodeBinder.clear();
    SHRNodeBinder(dest->Nodes, src->Nodes);

    SHRCopyNode(dest->Nodes, src->Nodes);
    SHRCopyAnimation(dest->Animations, src->Animations);
}
void SHRCopyNode(FBXNode* dest, const FBXNode* src){
    static const char __name_of_this_func[] = "SHRCopyNode(FBXNode*, const FBXNode*)";


    if(dest && src){
        const auto srcID = src->getID();
        if(srcID != dest->getID()){
            SHRPushErrorMessage("destination and source must have the same members", __name_of_this_func);
            return;
        }

        { // FBXNode member
            strcpy(dest->Name, src->Name);

            dest->TransformMatrix = src->TransformMatrix;
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

            for(auto* p0 = dest_c->SkinInfos.Values; FBX_PTRDIFFU(p0 - dest_c->SkinInfos.Values) < dest_c->SkinInfos.Length; ++p0){
                for(auto* p1 = p0->Values; FBX_PTRDIFFU(p1 - p0->Values) < p0->Length; ++p1){
                    auto f = ins_nodeBinder.find(p1->BindNode);

                    if(f != ins_nodeBinder.cend())
                        p1->BindNode = f->second;
                    else
                        SHRPushErrorMessage("an error occurred while binding node pointer on SkinInfos", __name_of_this_func);
                }
            }

            for(auto* p = dest_c->SkinDeforms.Values; FBX_PTRDIFFU(p - dest_c->SkinDeforms.Values) < dest_c->SkinDeforms.Length; ++p){
                auto f = ins_nodeBinder.find(p->TargetNode);

                if(f != ins_nodeBinder.cend())
                    p->TargetNode = f->second;
                else
                    SHRPushErrorMessage("an error occurred while binding node pointer on SkinDeforms", __name_of_this_func);
            }
        }

        SHRCopyNode(dest->Child, src->Child);
        SHRCopyNode(dest->Sibling, src->Sibling);
    }
    else if(dest && (!src))
        SHRPushErrorMessage("destination and source must have value. but source is null", __name_of_this_func);
    else if((!dest) && src)
        SHRPushErrorMessage("destination and source must have value. but destination is null", __name_of_this_func);
}
void SHRCopyAnimation(FBXAnimation* dest, const FBXAnimation* src){
    static const char __name_of_this_func[] = "SHRCopyAnimation(FBXNode*, const FBXNode*)";


    if(dest && src){
        SHRCopyAnimation(dest->Next, src->Next);
    }
    else if(dest && (!src))
        SHRPushErrorMessage("destination and source must have value. but source is null", __name_of_this_func);
    else if((!dest) && src)
        SHRPushErrorMessage("destination and source must have value. but destination is null", __name_of_this_func);
}