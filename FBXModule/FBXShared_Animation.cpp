/**
* @file FBXShared_Animation.cpp
* @date 2019/04/19
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <eastl/set.h>
#include <eastl/unordered_map.h>

#include <FBXAssign.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


using namespace fbxsdk;


struct _AnimationData_wrapper{
    FBXAnimation* ExportAnimations;
    AnimationData AnimationData;
};

using _Timestamp = eastl::set<FbxTime>;

static eastl::unordered_map<FbxAnimStack*, _AnimationData_wrapper> ins_AnimationFinder;


static inline void ins_updateTimestamp(FbxAnimCurve* kAnimCurve, _Timestamp& timestamp){
    if(!kAnimCurve)
        return;

    for(auto e = kAnimCurve->KeyGetCount(), i = 0; i < e; ++i){
        auto kTime = kAnimCurve->KeyGetTime(i);

        timestamp.emplace(kTime);
    }
}


static void ins_addAnimationRecursive(
    FbxAnimLayer* kAnimLayer,
    FbxNode* kNode,
    AnimationList& animationList
)
{
    { // translation
        _Timestamp timestamp;
        {
            auto* kCurveX = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
            ins_updateTimestamp(kCurveX, timestamp);

            auto* kCurveY = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
            ins_updateTimestamp(kCurveY, timestamp);

            auto* kCurveZ = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
            ins_updateTimestamp(kCurveZ, timestamp);
        }

        
    }

    { // rotation
        _Timestamp timestamp;
        {
            auto* kCurveX = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
            ins_updateTimestamp(kCurveX, timestamp);

            auto* kCurveY = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
            ins_updateTimestamp(kCurveY, timestamp);

            auto* kCurveZ = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
            ins_updateTimestamp(kCurveZ, timestamp);
        }
    }

    { // scaling
        _Timestamp timestamp;
        {
            auto* kCurveX = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
            ins_updateTimestamp(kCurveX, timestamp);

            auto* kCurveY = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
            ins_updateTimestamp(kCurveY, timestamp);

            auto* kCurveZ = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
            ins_updateTimestamp(kCurveZ, timestamp);
        }
    }

    for(auto eNode = kNode->GetChildCount(), iNode = 0; iNode < eNode; ++iNode){
        auto* kChildNode = kNode->GetChild(iNode);
        if(!kChildNode)
            continue;

        ins_addAnimationRecursive(
            kAnimLayer,
            kChildNode,
            animationList
        );
    }
}


bool SHRLoadAnimation(FbxManager* kSDKManager, FbxScene* kScene){
    auto** pAnim = &shr_root->Animations;
    for(auto eAnimStack = kScene->GetSrcObjectCount<FbxAnimStack>(), iAnimStack = 0; iAnimStack < eAnimStack; ++iAnimStack){
        auto* kAnimStack = kScene->GetSrcObject<FbxAnimStack>(iAnimStack);
        if(!kAnimStack)
            continue;

        for(auto eAnimLayer = kAnimStack->GetMemberCount<FbxAnimLayer>(), iAnimLayer = 0; iAnimLayer < eAnimLayer; ++iAnimLayer){
            auto* kAnimLayer = kAnimStack->GetMember<FbxAnimLayer>(iAnimLayer);
            if(!kAnimLayer)
                continue;


        }


        // we will use keyframes only from layer 0


        auto* kAnimLayer = kAnimStack->GetMember<FbxAnimLayer>(0);
        if(!kAnimLayer)
            continue;

        AnimationData genAnimData;

        {
            genAnimData.strName = kAnimStack->GetName();
        }

        ins_addAnimationRecursive(
            kAnimLayer,
            kScene->GetRootNode(),
            genAnimData.animationList
        );

        {
            _AnimationData_wrapper _new;

            (*pAnim) = FBXNew<FBXAnimation>();

            _new.ExportAnimations = (*pAnim);
            _new.AnimationData = eastl::move(genAnimData);

            ins_AnimationFinder.emplace(kAnimStack, eastl::move(_new));

            pAnim = &((*pAnim)->Next);
        }
    }

    return true;
}
