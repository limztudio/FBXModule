/**
 * @file FBXShared.cpp
 * @date 2020/05/08
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#include "stdafx.h"

#include "FBXUtilites.h"
#include "FBXShared.h"


static fbx_unordered_map<const FBXNode*, FBXNode*, PointerHasher<const FBXNode*>> ins_nodeBinder;


static void ins_initNodeBinder(FBXNode* dest, const FBXNode* src){
    if(dest && src){
        ins_nodeBinder[src] = dest;

        ins_initNodeBinder(dest->Child, src->Child);
        ins_initNodeBinder(dest->Sibling, src->Sibling);
    }
}


void SHRRebindRoot(FBXRoot* dest, const FBXRoot* src){
    ins_nodeBinder.clear();
    ins_initNodeBinder(dest->Nodes, src->Nodes);

    SHRRebindNode(dest->Nodes, src->Nodes);

    for(size_t idxAnimation = 0; idxAnimation < dest->Animations.Length; ++idxAnimation)
        SHRRebindAnimation(dest->Animations.Values[idxAnimation], src->Animations.Values[idxAnimation]);
}
void SHRRebindNode(FBXNode* dest, const FBXNode* src){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRRebindNode(FBXNode*, const FBXNode*)");


    if(dest && src){
        const auto srcID = src->getID();
        if(srcID != dest->getID()){
            SHRPushErrorMessage(FBX_TEXT("destination and source must have the same members"), __name_of_this_func);
            return;
        }

        if(FBXTypeHasMember(srcID, FBXType::FBXType_SkinnedMesh)){
            auto* dest_c = static_cast<FBXSkinnedMesh*>(dest);

            for(auto* p0 = dest_c->BoneCombinations.Values; FBX_PTRDIFFU(p0 - dest_c->BoneCombinations.Values) < dest_c->BoneCombinations.Length; ++p0){
                for(auto* p1 = p0->Values; FBX_PTRDIFFU(p1 - p0->Values) < p0->Length; ++p1){
                    auto f = ins_nodeBinder.find(*p1);

                    if(f != ins_nodeBinder.cend())
                        *p1 = f->second;
                    else
                        SHRPushErrorMessage(FBX_TEXT("an error occurred while binding node pointer on BoneCombinations"), __name_of_this_func);
                }
            }
            for(auto* p0 = dest_c->SkinInfos.Values; FBX_PTRDIFFU(p0 - dest_c->SkinInfos.Values) < dest_c->SkinInfos.Length; ++p0){
                for(auto* p1 = p0->Values; FBX_PTRDIFFU(p1 - p0->Values) < p0->Length; ++p1){
                    auto f = ins_nodeBinder.find(p1->BindNode);

                    if(f != ins_nodeBinder.cend())
                        p1->BindNode = f->second;
                    else
                        SHRPushErrorMessage(FBX_TEXT("an error occurred while binding node pointer on SkinInfos"), __name_of_this_func);
                }
            }
            for(auto* p = dest_c->SkinDeforms.Values; FBX_PTRDIFFU(p - dest_c->SkinDeforms.Values) < dest_c->SkinDeforms.Length; ++p){
                auto f = ins_nodeBinder.find(p->TargetNode);

                if(f != ins_nodeBinder.cend())
                    p->TargetNode = f->second;
                else
                    SHRPushErrorMessage(FBX_TEXT("an error occurred while binding node pointer on SkinDeforms"), __name_of_this_func);
            }
        }

        SHRRebindNode(dest->Child, src->Child);
        SHRRebindNode(dest->Sibling, src->Sibling);
    }
    else if(dest && (!src))
        SHRPushErrorMessage(FBX_TEXT("destination and source must have value. but source is null"), __name_of_this_func);
    else if((!dest) && src)
        SHRPushErrorMessage(FBX_TEXT("destination and source must have value. but destination is null"), __name_of_this_func);
}
void SHRRebindAnimation(FBXAnimation& dest, const FBXAnimation& src){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRRebindAnimation(FBXAnimation&, const FBXAnimation&)");


    for(auto* p = dest.AnimationNodes.Values; FBX_PTRDIFFU(p - dest.AnimationNodes.Values) < dest.AnimationNodes.Length; ++p){
        auto f = ins_nodeBinder.find(p->BindNode);

        if(f != ins_nodeBinder.cend())
            p->BindNode = f->second;
        else
            SHRPushErrorMessage(FBX_TEXT("an error occurred while binding node pointer on Animation"), __name_of_this_func);
    }
}
