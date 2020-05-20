/**
* @file FBXShared_Animation.cpp
* @date 2019/04/19
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <eastl/set.h>
#include <eastl/unordered_map.h>
#include <eastl/slist.h>

#include <FBXAssign.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


using namespace fbxsdk;


struct _Marker{
    FbxAnimCurveKey curveKey;
};
static inline bool operator==(const _Marker& lhs, const _Marker& rhs){
    return (lhs.curveKey.GetTime() == rhs.curveKey.GetTime());
}
static inline bool operator<(const _Marker& lhs, const _Marker& rhs){
    return (lhs.curveKey.GetTime() < rhs.curveKey.GetTime());
}
using _Timestamp = eastl::set<_Marker>;


static eastl::vector<AnimationData> ins_animations;


static inline void ins_updateTimestamp(FbxAnimCurve* kAnimCurve, _Timestamp& timestamp){
    if(!kAnimCurve)
        return;

    for(auto e = kAnimCurve->KeyGetCount(), i = 0; i < e; ++i){
        _Marker newMaker;
        newMaker.curveKey = kAnimCurve->KeyGet(i);
        timestamp.emplace(eastl::move(newMaker));
    }
}


static void ins_collectNodes(AnimationNodes& nodeTable, FbxNode* kNode){
    if(kNode){
        nodeTable.emplace_back(kNode);

        for(auto e = kNode->GetChildCount(), i = 0; i < e; ++i)
            ins_collectNodes(nodeTable, kNode->GetChild(i));
    }
}

static inline void ins_addAnimation(
    FbxAnimLayer* kAnimLayer,
    FbxNode* kNode,
    AnimationLayer& animationLayer
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

        {
            auto& curKeys = animationLayer.translationKeys;

            curKeys.clear();
            curKeys.reserve(timestamp.size());
            for(const auto& kMarker : timestamp){
                auto& kCurveKey = kMarker.curveKey;
                FbxDouble3 kVal = GetLocalTransform(kNode, kCurveKey.GetTime()).GetT();
                curKeys.emplace_back(eastl::move(kCurveKey), eastl::move(kVal));
            }
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

        {
            auto& curKeys = animationLayer.rotationKeys;

            curKeys.clear();
            curKeys.reserve(timestamp.size());
            for(const auto& kMarker : timestamp){
                auto& kCurveKey = kMarker.curveKey;
                FbxDouble4 kVal = GetLocalTransform(kNode, kCurveKey.GetTime()).GetQ();
                curKeys.emplace_back(eastl::move(kCurveKey), eastl::move(kVal));
            }
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

        {
            auto& curKeys = animationLayer.scaleKeys;

            curKeys.clear();
            curKeys.reserve(timestamp.size());
            for(const auto& kMarker : timestamp){
                auto& kCurveKey = kMarker.curveKey;
                FbxDouble3 kVal = GetLocalTransform(kNode, kCurveKey.GetTime()).GetS();
                curKeys.emplace_back(eastl::move(kCurveKey), eastl::move(kVal));
            }
        }
    }
}

template<typename EXPORT_TYPE, typename FBX_TYPE>
static inline void ins_convAnimationKey(EXPORT_TYPE& expKey, FBX_TYPE& fbxKey){
    expKey.Time = decltype(expKey.Time)(fbxKey.curveKey.GetTime().GetSecondDouble());

    switch(fbxKey.curveKey.GetInterpolation()){
    case FbxAnimCurveDef::eInterpolationConstant:
        expKey.InterpolationType = FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped;
        break;
    case FbxAnimCurveDef::eInterpolationLinear:
        expKey.InterpolationType = FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear;
        break;
    case FbxAnimCurveDef::eInterpolationCubic:
        expKey.InterpolationType = FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear;
        break;
    }

    CopyArrayData(expKey.Value.Values, fbxKey.value.mData);
}


bool SHRLoadAnimation(FbxManager* kSDKManager, FbxScene* kScene, const AnimationNodes& kNodeTable){
    ins_animations.resize((size_t)kScene->GetSrcObjectCount<FbxAnimStack>());
    for(auto edxAnimStack = (int)ins_animations.size(), idxAnimStack = 0; idxAnimStack < edxAnimStack; ++idxAnimStack){
        auto* kAnimStack = kScene->GetSrcObject<FbxAnimStack>(idxAnimStack);
        if(!kAnimStack)
            continue;

        const auto edxAnimLayer = kAnimStack->GetMemberCount<FbxAnimLayer>();

        auto& iAnimation = ins_animations[idxAnimStack];
        {
            iAnimation.strName = kAnimStack->GetName();

            iAnimation.nodes.clear();
            iAnimation.nodes.reserve(kNodeTable.size());
            for(auto* kNode : kNodeTable){
                AnimationLayerContainer newLayerContainer;

                newLayerContainer.bindNode = kNode;

                newLayerContainer.layers.clear();
                newLayerContainer.layers.reserve(edxAnimLayer);

                iAnimation.nodes.emplace_back(eastl::move(newLayerContainer));
            }
        }

        for(auto idxAnimLayer = decltype(edxAnimLayer){ 0 }; idxAnimLayer < edxAnimLayer; ++idxAnimLayer){
            auto* kAnimLayer = kAnimStack->GetMember<FbxAnimLayer>(idxAnimLayer);
            if(!kAnimLayer)
                continue;

            for(auto& iNode : iAnimation.nodes){
                auto* kNode = iNode.bindNode;

                iNode.layers.emplace_back(AnimationLayer());
                auto& iAnimLayer = *iNode.layers.rbegin();

                ins_addAnimation(
                    kAnimLayer,
                    kNode,
                    iAnimLayer
                );
            }
        }
    }

    return true;
}
bool SHRLoadAnimations(FbxManager* kSDKManager, FbxScene* kScene, const FbxNodeToExportNode& fbxNodeToExportNode){
    static const char __name_of_this_func[] = "SHRLoadAnimations(FbxManager*, FbxScene*, const FbxNodeToExportNode&)";


    AnimationNodes kNodeTable;
    {
        kNodeTable.reserve(fbxNodeToExportNode.size());
        ins_collectNodes(kNodeTable, kScene->GetRootNode());
    }

    ins_animations.clear();
    SHRLoadAnimation(kSDKManager, kScene, kNodeTable);

    auto** pNewAnimation = &shr_root->Animations;
    for(auto& iAnimation : ins_animations){
        (*pNewAnimation) = FBXNew<FBXAnimation>();
        auto* pAnimation = (*pNewAnimation);

        {
            const auto lenName = iAnimation.strName.length();
            pAnimation->Name = FBXAllocate<char>(lenName + 1);
            CopyArrayData(pAnimation->Name, iAnimation.strName.c_str(), lenName);
            pAnimation->Name[lenName] = 0;
        }

        pAnimation->AnimationNodes.Assign(iAnimation.nodes.size());
        for(size_t idxNode = 0; idxNode < pAnimation->AnimationNodes.Length; ++idxNode){
            auto& iNode = iAnimation.nodes[idxNode];
            auto* pNode = &pAnimation->AnimationNodes.Values[idxNode];

            const eastl::string strNodeName = iNode.bindNode->GetName();

            {
                auto f = fbxNodeToExportNode.find(iNode.bindNode);
                if(f == fbxNodeToExportNode.end()){
                    eastl::string msg = "bind node not found";
                    msg += "(errored in \"";
                    msg += strNodeName;
                    msg += "\")";
                    SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                    return false;
                }

                pNode->BindNode = f->second;
            }

            pNode->LayeredElements.Assign(iNode.layers.size());
            for(size_t idxLayer = 0; idxLayer < pNode->LayeredElements.Length; ++idxLayer){
                auto& iLayer = iNode.layers[idxLayer];
                auto* pLayer = &pNode->LayeredElements.Values[idxLayer];

                pLayer->ScaleKeys.Assign(iLayer.scaleKeys.size());
                for(size_t idxKey = 0; idxKey < pLayer->ScaleKeys.Length; ++idxKey){
                    auto& iKey = iLayer.scaleKeys[idxKey];
                    auto* pKey = &pLayer->ScaleKeys.Values[idxKey];

                    ins_convAnimationKey(*pKey, iKey);
                }

                pLayer->RotationKeys.Assign(iLayer.rotationKeys.size());
                for(size_t idxKey = 0; idxKey < pLayer->RotationKeys.Length; ++idxKey){
                    auto& iKey = iLayer.rotationKeys[idxKey];
                    auto* pKey = &pLayer->RotationKeys.Values[idxKey];

                    ins_convAnimationKey(*pKey, iKey);
                }

                pLayer->TranslationKeys.Assign(iLayer.translationKeys.size());
                for(size_t idxKey = 0; idxKey < pLayer->TranslationKeys.Length; ++idxKey){
                    auto& iKey = iLayer.translationKeys[idxKey];
                    auto* pKey = &pLayer->TranslationKeys.Values[idxKey];

                    ins_convAnimationKey(*pKey, iKey);
                }
            }
        }

        pNewAnimation = &((*pNewAnimation)->Next);
    }

    return true;
}
