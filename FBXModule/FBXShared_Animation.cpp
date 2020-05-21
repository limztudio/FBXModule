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


static eastl::vector<AnimationStack> ins_animationStacks;


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

static inline FbxAnimCurveDef::EInterpolationType ins_convInterpolationType(FBXAnimationInterpolationType type){
    switch(type){
    case FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped:
        return FbxAnimCurveDef::eInterpolationConstant;
    case FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear:
        return FbxAnimCurveDef::eInterpolationLinear;
    }

    return FbxAnimCurveDef::eInterpolationConstant;
}


bool SHRLoadAnimation(FbxManager* kSDKManager, FbxScene* kScene, const AnimationNodes& kNodeTable){
    static const char __name_of_this_func[] = "SHRLoadAnimation(FbxManager*, FbxScene*, const AnimationNodes&)";


    ins_animationStacks.resize((size_t)kScene->GetSrcObjectCount<FbxAnimStack>());
    for(auto edxAnimStack = (int)ins_animationStacks.size(), idxAnimStack = 0; idxAnimStack < edxAnimStack; ++idxAnimStack){
        auto* kAnimStack = kScene->GetSrcObject<FbxAnimStack>(idxAnimStack);
        if(!kAnimStack)
            continue;

        const eastl::string strStackName = kAnimStack->GetName();

        auto& iAnimStack = ins_animationStacks[idxAnimStack];

        iAnimStack.strName = strStackName.c_str();

        iAnimStack.layers.resize((size_t)kAnimStack->GetMemberCount<FbxAnimLayer>());
        for(auto edxAnimLayer = (int)iAnimStack.layers.size(), idxAnimLayer = 0; idxAnimLayer < edxAnimLayer; ++idxAnimLayer){
            auto* kAnimLayer = kAnimStack->GetMember<FbxAnimLayer>(idxAnimLayer);
            if(!kAnimLayer)
                continue;

            const eastl::string strLayerName = kAnimLayer->GetName();

            auto& iAnimLayer = iAnimStack.layers[idxAnimLayer];

            iAnimLayer.strName = strLayerName.c_str();

            iAnimLayer.nodes.clear();
            iAnimLayer.nodes.reserve(kNodeTable.size());
            for(auto* kNode : kNodeTable){
                AnimationNode newNodes;

                newNodes.bindNode = kNode;

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
                        auto& curKeys = newNodes.translationKeys;

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
                        auto& curKeys = newNodes.rotationKeys;

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
                        auto& curKeys = newNodes.scalingKeys;

                        curKeys.clear();
                        curKeys.reserve(timestamp.size());
                        for(const auto& kMarker : timestamp){
                            auto& kCurveKey = kMarker.curveKey;

                            FbxDouble3 kVal = GetLocalTransform(kNode, kCurveKey.GetTime()).GetS();
                            curKeys.emplace_back(eastl::move(kCurveKey), eastl::move(kVal));
                        }
                    }
                }

                if(!newNodes.isEmpty())
                    iAnimLayer.nodes.emplace_back(eastl::move(newNodes));
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

    ins_animationStacks.clear();
    SHRLoadAnimation(kSDKManager, kScene, kNodeTable);

    auto** pNewAnimation = &shr_root->Animations;
    for(auto& iAnimation : ins_animationStacks){
        (*pNewAnimation) = FBXNew<FBXAnimation>();
        auto* pAnimation = (*pNewAnimation);

        {
            const auto lenName = iAnimation.strName.length();
            pAnimation->Name.Assign(lenName + 1);
            CopyArrayData(pAnimation->Name.Values, iAnimation.strName.c_str(), lenName);
            pAnimation->Name.Values[lenName] = 0;
        }

        pAnimation->AnimationLayers.Assign(iAnimation.layers.size());
        for(size_t idxLayer = 0; idxLayer < pAnimation->AnimationLayers.Length; ++idxLayer){
            auto& iLayer = iAnimation.layers[idxLayer];
            auto* pLayer = &pAnimation->AnimationLayers.Values[idxLayer];

            {
                const auto lenName = iLayer.strName.length();
                pLayer->Name.Assign(lenName + 1);
                CopyArrayData(pLayer->Name.Values, iLayer.strName.c_str(), lenName);
                pLayer->Name.Values[lenName] = 0;
            }
            
            pLayer->AnimationNodes.Assign(iLayer.nodes.size());
            for(size_t idxNode = 0; idxNode < pLayer->AnimationNodes.Length; ++idxNode){
                auto& iNode = iLayer.nodes[idxNode];
                auto* pNode = &pLayer->AnimationNodes.Values[idxNode];

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

                pNode->ScalingKeys.Assign(iNode.scalingKeys.size());
                for(size_t idxKey = 0; idxKey < pNode->ScalingKeys.Length; ++idxKey){
                    auto& iKey = iNode.scalingKeys[idxKey];
                    auto* pKey = &pNode->ScalingKeys.Values[idxKey];

                    ins_convAnimationKey(*pKey, iKey);
                }

                pNode->RotationKeys.Assign(iNode.rotationKeys.size());
                for(size_t idxKey = 0; idxKey < pNode->RotationKeys.Length; ++idxKey){
                    auto& iKey = iNode.rotationKeys[idxKey];
                    auto* pKey = &pNode->RotationKeys.Values[idxKey];

                    ins_convAnimationKey(*pKey, iKey);
                }

                pNode->TranslationKeys.Assign(iNode.translationKeys.size());
                for(size_t idxKey = 0; idxKey < pNode->TranslationKeys.Length; ++idxKey){
                    auto& iKey = iNode.translationKeys[idxKey];
                    auto* pKey = &pNode->TranslationKeys.Values[idxKey];

                    ins_convAnimationKey(*pKey, iKey);
                }
            }
        }

        pNewAnimation = &((*pNewAnimation)->Next);
    }

    return true;
}

bool SHRStoreAnimation(FbxManager* kSDKManager, FbxScene* kScene, const ImportNodeToFbxNode& importNodeToFbxNode, const FBXAnimation* pAnimStack){
    static const char __name_of_this_func[] = "SHRStoreAnimation(FbxManager*, FbxScene*, const ImportNodeToFbxNode&, const FBXAnimation*)";


    if(pAnimStack){
        const eastl::string strStackName = pAnimStack->Name.Values;

        auto* kAnimStack = FbxAnimStack::Create(kScene, strStackName.c_str());
        if(!kAnimStack){
            eastl::string msg = "failed to create FbxAnimStack";
            msg += "(errored in \"";
            msg += strStackName;
            msg += "\")";
            SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
            return false;
        }

        for(auto* pAnimLayer = pAnimStack->AnimationLayers.Values; FBX_PTRDIFFU(pAnimLayer - pAnimStack->AnimationLayers.Values) < pAnimStack->AnimationLayers.Length; ++pAnimLayer){
            const eastl::string strLayerName = pAnimLayer->Name.Values;

            auto* kAnimLayer = FbxAnimLayer::Create(kScene, strLayerName.c_str());
            if(!kAnimLayer){
                eastl::string msg = "failed to create FbxAnimLayer";
                msg += "(errored in \"";
                msg += strLayerName;
                msg += "\" of stack \"";
                msg += strStackName;
                msg += "\")";
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                return false;
            }

            for(auto* pAnimNode = pAnimLayer->AnimationNodes.Values; FBX_PTRDIFFU(pAnimNode - pAnimLayer->AnimationNodes.Values) < pAnimLayer->AnimationNodes.Length; ++pAnimNode){
                const eastl::string strNodeName = pAnimNode->BindNode->Name.Values;

                FbxNode* kNode = nullptr;
                {
                    auto f = importNodeToFbxNode.find(pAnimNode->BindNode);
                    if(f == importNodeToFbxNode.cend()){
                        eastl::string msg = "an error occurred while adding key frame. cannot find bind node";
                        msg += "(errored in \"";
                        msg += strNodeName;
                        msg += "\")";
                        SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                        return false;
                    }
                    kNode = f->second;
                }

                { // translation
                    auto* kCurveX = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
                    if(!kCurveX){
                        eastl::string msg = "failed to get curve of X translation component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveY = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    if(!kCurveY){
                        eastl::string msg = "failed to get curve of Y translation component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveZ = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
                    if(!kCurveZ){
                        eastl::string msg = "failed to get curve of Z translation component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                        return false;
                    }

                    {
                        kCurveX->KeyModifyBegin();
                        kCurveY->KeyModifyBegin();
                        kCurveZ->KeyModifyBegin();
                    }

                    const auto* pKeyList = &pAnimNode->TranslationKeys;
                    for(auto* pKey = pKeyList->Values; FBX_PTRDIFFU(pKey - pKeyList->Values) < pKeyList->Length; ++pKey){
                        auto curType = ins_convInterpolationType(pKey->InterpolationType);
                        FbxTime curTime;
                        int curIndex;

                        curTime.SetSecondDouble(pKey->Time);

                        curIndex = kCurveX->KeyAdd(curTime);
                        kCurveX->KeySetValue(curIndex, pKey->Value.Values[0]);
                        kCurveX->KeySetInterpolation(curIndex, curType);

                        curIndex = kCurveY->KeyAdd(curTime);
                        kCurveY->KeySetValue(curIndex, pKey->Value.Values[1]);
                        kCurveY->KeySetInterpolation(curIndex, curType);

                        curIndex = kCurveZ->KeyAdd(curTime);
                        kCurveZ->KeySetValue(curIndex, pKey->Value.Values[2]);
                        kCurveZ->KeySetInterpolation(curIndex, curType);
                    }

                    {
                        kCurveX->KeyModifyEnd();
                        kCurveY->KeyModifyEnd();
                        kCurveZ->KeyModifyEnd();
                    }
                }

                { // rotation
                    auto* kCurveX = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
                    if(!kCurveX){
                        eastl::string msg = "failed to get curve of X rotation component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveY = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    if(!kCurveY){
                        eastl::string msg = "failed to get curve of Y rotation component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveZ = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
                    if(!kCurveZ){
                        eastl::string msg = "failed to get curve of Z rotation component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                        return false;
                    }

                    {
                        kCurveX->KeyModifyBegin();
                        kCurveY->KeyModifyBegin();
                        kCurveZ->KeyModifyBegin();
                    }

                    const auto* pKeyList = &pAnimNode->RotationKeys;
                    for(auto* pKey = pKeyList->Values; FBX_PTRDIFFU(pKey - pKeyList->Values) < pKeyList->Length; ++pKey){
                        auto curType = ins_convInterpolationType(pKey->InterpolationType);
                        Float3 curVal = MakeRotation(Float4(pKey->Value.Values));
                        FbxTime curTime;
                        int curIndex;

                        curTime.SetSecondDouble(pKey->Time);

                        curIndex = kCurveX->KeyAdd(curTime);
                        kCurveX->KeySetValue(curIndex, curVal.x);
                        kCurveX->KeySetInterpolation(curIndex, curType);

                        curIndex = kCurveY->KeyAdd(curTime);
                        kCurveY->KeySetValue(curIndex, curVal.y);
                        kCurveY->KeySetInterpolation(curIndex, curType);

                        curIndex = kCurveZ->KeyAdd(curTime);
                        kCurveZ->KeySetValue(curIndex, curVal.z);
                        kCurveZ->KeySetInterpolation(curIndex, curType);
                    }

                    {
                        kCurveX->KeyModifyEnd();
                        kCurveY->KeyModifyEnd();
                        kCurveZ->KeyModifyEnd();
                    }
                }

                { // scaling
                    auto* kCurveX = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
                    if(!kCurveX){
                        eastl::string msg = "failed to get curve of X scaling component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveY = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    if(!kCurveY){
                        eastl::string msg = "failed to get curve of Y scaling component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveZ = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
                    if(!kCurveZ){
                        eastl::string msg = "failed to get curve of Z scaling component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                        return false;
                    }

                    {
                        kCurveX->KeyModifyBegin();
                        kCurveY->KeyModifyBegin();
                        kCurveZ->KeyModifyBegin();
                    }

                    const auto* pKeyList = &pAnimNode->ScalingKeys;
                    for(auto* pKey = pKeyList->Values; FBX_PTRDIFFU(pKey - pKeyList->Values) < pKeyList->Length; ++pKey){
                        auto curType = ins_convInterpolationType(pKey->InterpolationType);
                        FbxTime curTime;
                        int curIndex;

                        curTime.SetSecondDouble(pKey->Time);

                        curIndex = kCurveX->KeyAdd(curTime);
                        kCurveX->KeySetValue(curIndex, pKey->Value.Values[0]);
                        kCurveX->KeySetInterpolation(curIndex, curType);

                        curIndex = kCurveY->KeyAdd(curTime);
                        kCurveY->KeySetValue(curIndex, pKey->Value.Values[1]);
                        kCurveY->KeySetInterpolation(curIndex, curType);

                        curIndex = kCurveZ->KeyAdd(curTime);
                        kCurveZ->KeySetValue(curIndex, pKey->Value.Values[2]);
                        kCurveZ->KeySetInterpolation(curIndex, curType);
                    }

                    {
                        kCurveX->KeyModifyEnd();
                        kCurveY->KeyModifyEnd();
                        kCurveZ->KeyModifyEnd();
                    }
                }
            }

            if(!kAnimStack->AddMember(kAnimLayer)){
                eastl::string msg = "failed to add FbxAnimLayer member to FbxAnimStack";
                msg += "(errored in \"";
                msg += strLayerName;
                msg += "\" of stack \"";
                msg += strStackName;
                msg += "\")";
                SHRPushErrorMessage(eastl::move(msg), __name_of_this_func);
                return false;
            }
        }



        if(!SHRStoreAnimation(kSDKManager, kScene, importNodeToFbxNode, pAnimStack->Next))
            return false;
    }

    return true;
}
bool SHRStoreAnimations(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const ImportNodeToFbxNode& importNodeToFbxNode, const FBXAnimation* pRootAnimStack){
    static const char __name_of_this_func[] = "SHRStoreAnimations(FbxManager*, FbxScene*, const ImportNodeToFbxNode&, const FBXAnimation*)";


    if(!SHRStoreAnimation(kSDKManager, kScene, importNodeToFbxNode, pRootAnimStack))
        return false;

    return true;
}
