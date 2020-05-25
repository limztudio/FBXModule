/**
* @file FBXShared_Animation.cpp
* @date 2019/04/19
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <set>

#include <FBXAssign.hpp>

#include "FBXUtilites.h"
#include "FBXMath.h"
#include "FBXShared.h"


using namespace fbxsdk;


static std::vector<AnimationStack> ins_animationStacks;

static std::set<FbxTime> ins_animationKeyFrames[3];


template<typename KEY_TABLE, typename VALUE>
static inline void ins_keyTypeOptimze(KEY_TABLE& keyTable, const VALUE& kVal){
    const auto keycount = keyTable.size();
    if(keycount > 1){
        auto& iPrevKey = keyTable[keycount - 2];
        const auto& kPrevVal = iPrevKey.value;

        if(kPrevVal == kVal)
            iPrevKey.type = FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped;
    }
}

template<typename KEY_TABLE>
static inline void ins_keyReduce(KEY_TABLE& keyTable){
    if(keyTable.size() > 2){
        size_t idxPivot = 1;
        while((keyTable.size() > 2) && ((idxPivot + 1) < keyTable.size())){
            const auto& iLhs = keyTable[idxPivot - 1];
            const auto& iCur = keyTable[idxPivot];
            const auto& iRhs = keyTable[idxPivot + 1];

            if((iLhs.value == iRhs.value) && (iLhs.value == iCur.value)){
                auto itr = keyTable.begin();
                std::advance(itr, idxPivot);

                keyTable.erase(itr);
                continue;
            }

            ++idxPivot;
        }
    }
}

static inline void ins_updateTimestamp(FbxAnimCurve* kAnimCurve, size_t idx){
    if(!kAnimCurve)
        return;

    for(auto e = kAnimCurve->KeyGetCount(), i = 0; i < e; ++i)
        ins_animationKeyFrames[idx].emplace(kAnimCurve->KeyGet(i).GetTime());
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
    expKey.Time = decltype(expKey.Time)(fbxKey.time.GetSecondDouble());
    expKey.InterpolationType = fbxKey.type;

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


    // attempt: the transform of key frames are able to earn from Evaluator.
    // but since Evaluator looks it has no relation with FbxAnimStack or FbxAnimLayer(well, seems FbxAnimLayer are calculated on that so far)
    // and Evaluator shows up only the animation of default FbxAnimStack(and guess the default will be 0 index of FbxAnimStack).
    // so what I'm gonna do is, disconnect all animation stacks from scene, and re-connect only one of them by iterating animation stacks.

    // well, it ain't work properly...
    // but later, try to connect animation stack object as dstObject

    const auto edxAnimStack = kScene->GetSrcObjectCount<FbxAnimStack>();

    ins_animationStacks.clear();
    ins_animationStacks.reserve((size_t)edxAnimStack);

    for(auto idxAnimStack = decltype(edxAnimStack){ 0 }; idxAnimStack < edxAnimStack; ++idxAnimStack){
        auto* kAnimStack = kScene->GetSrcObject<FbxAnimStack>(idxAnimStack);
        if(!kAnimStack)
            continue;

        kScene->SetCurrentAnimationStack(kAnimStack);

        ins_animationStacks.emplace_back(AnimationStack());
        auto& iAnimStack = *ins_animationStacks.rbegin();

        iAnimStack.animStack = kAnimStack;

        const std::string strStackName = kAnimStack->GetName();

        iAnimStack.nodes.clear();
        iAnimStack.nodes.reserve(kNodeTable.size());

        for(auto* kNode : kNodeTable){
            for(auto& keyTable : ins_animationKeyFrames)
                keyTable.clear();

            for(auto edxAnimLayer = kAnimStack->GetMemberCount<FbxAnimLayer>(), idxAnimLayer = 0; idxAnimLayer < edxAnimLayer; ++idxAnimLayer){
                auto* kAnimLayer = kAnimStack->GetMember<FbxAnimLayer>(idxAnimLayer);
                if(!kAnimLayer)
                    continue;

                { // translation
                    auto* kCurveX = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
                    ins_updateTimestamp(kCurveX, 0);

                    auto* kCurveY = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
                    ins_updateTimestamp(kCurveY, 0);

                    auto* kCurveZ = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
                    ins_updateTimestamp(kCurveZ, 0);
                }

                { // rotation
                    auto* kCurveX = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
                    ins_updateTimestamp(kCurveX, 1);

                    auto* kCurveY = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
                    ins_updateTimestamp(kCurveY, 1);

                    auto* kCurveZ = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
                    ins_updateTimestamp(kCurveZ, 1);
                }

                { // scaling
                    auto* kCurveX = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
                    ins_updateTimestamp(kCurveX, 2);

                    auto* kCurveY = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
                    ins_updateTimestamp(kCurveY, 2);

                    auto* kCurveZ = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
                    ins_updateTimestamp(kCurveZ, 2);
                }
            }

            auto kDefaultMat = GetLocalTransform(kNode);
            AnimationNode newNodes;

            newNodes.bindNode = kNode;

            { // translation
                newNodes.translationKeys.clear();
                newNodes.translationKeys.reserve(ins_animationKeyFrames[0].size());
                for(const auto& kTime : ins_animationKeyFrames[0]){
                    auto kMat = GetLocalTransform(kNode, kTime);
                    auto kValRaw = kMat.GetT();
                    FbxDouble3 kVal(kValRaw[0], kValRaw[1], kValRaw[2]);
                    newNodes.translationKeys.emplace_back(kTime, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear, kVal);
                    ins_keyTypeOptimze(newNodes.translationKeys, kVal);
                }

                ins_keyReduce(newNodes.translationKeys);

                if(newNodes.translationKeys.empty()){
                    newNodes.translationKeys.reserve(1);
                    auto kValRaw = kDefaultMat.GetT();
                    FbxDouble3 kVal(kValRaw[0], kValRaw[1], kValRaw[2]);
                    newNodes.translationKeys.emplace_back(0, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped, kVal);
                }
            }

            { // rotation
                newNodes.rotationKeys.clear();
                newNodes.rotationKeys.reserve(ins_animationKeyFrames[0].size());
                for(const auto& kTime : ins_animationKeyFrames[0]){
                    auto kMat = GetLocalTransform(kNode, kTime);
                    FbxDouble4 kVal = kMat.GetQ();
                    newNodes.rotationKeys.emplace_back(kTime, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear, kVal);
                    ins_keyTypeOptimze(newNodes.rotationKeys, kVal);
                }

                ins_keyReduce(newNodes.rotationKeys);

                if(newNodes.rotationKeys.empty()){
                    newNodes.rotationKeys.reserve(1);
                    auto kVal = kDefaultMat.GetQ();
                    newNodes.rotationKeys.emplace_back(0, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped, kVal);
                }
            }

            { // scaling
                newNodes.scalingKeys.clear();
                newNodes.scalingKeys.reserve(ins_animationKeyFrames[0].size());
                for(const auto& kTime : ins_animationKeyFrames[0]){
                    auto kMat = GetLocalTransform(kNode, kTime);
                    auto kValRaw = kMat.GetS();
                    FbxDouble3 kVal(kValRaw[0], kValRaw[1], kValRaw[2]);
                    newNodes.scalingKeys.emplace_back(kTime, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear, kVal);
                    ins_keyTypeOptimze(newNodes.scalingKeys, kVal);
                }

                ins_keyReduce(newNodes.scalingKeys);

                if(newNodes.scalingKeys.empty()){
                    newNodes.scalingKeys.reserve(1);
                    auto kValRaw = kDefaultMat.GetS();
                    FbxDouble3 kVal(kValRaw[0], kValRaw[1], kValRaw[2]);
                    newNodes.scalingKeys.emplace_back(0, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped, kVal);
                }
            }

            {
                newNodes.translationKeys.rbegin()->type = FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped;
                newNodes.rotationKeys.rbegin()->type = FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped;
                newNodes.scalingKeys.rbegin()->type = FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped;
            }

            iAnimStack.nodes.emplace_back(std::move(newNodes));
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

        const std::string strStackName = iAnimation.animStack->GetName();

        {
            const auto lenName = strStackName.length();
            pAnimation->Name.Assign(lenName + 1);
            CopyArrayData(pAnimation->Name.Values, strStackName.c_str(), lenName);
            pAnimation->Name.Values[lenName] = 0;
        }

        pAnimation->AnimationNodes.Assign(iAnimation.nodes.size());
        for(size_t idxNode = 0; idxNode < pAnimation->AnimationNodes.Length; ++idxNode){
            auto& iNode = iAnimation.nodes[idxNode];
            auto* pNode = &pAnimation->AnimationNodes.Values[idxNode];

            const std::string strNodeName = iNode.bindNode->GetName();

            {
                auto f = fbxNodeToExportNode.find(iNode.bindNode);
                if(f == fbxNodeToExportNode.end()){
                    std::string msg = "bind node not found";
                    msg += "(errored in \"";
                    msg += strNodeName;
                    msg += "\")";
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
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

        pNewAnimation = &((*pNewAnimation)->Next);
    }

    return true;
}

bool SHRStoreAnimation(FbxManager* kSDKManager, FbxScene* kScene, const ImportNodeToFbxNode& importNodeToFbxNode, const FBXAnimation* pAnimStack){
    static const char __name_of_this_func[] = "SHRStoreAnimation(FbxManager*, FbxScene*, const ImportNodeToFbxNode&, const FBXAnimation*)";


    if(pAnimStack){
        FbxAnimCurveFilterUnroll kFilterUnroll;

        const std::string strStackName = pAnimStack->Name.Values;

        auto* kAnimStack = FbxAnimStack::Create(kScene, strStackName.c_str());
        if(!kAnimStack){
            std::string msg = "failed to create FbxAnimStack";
            msg += "(errored in \"";
            msg += strStackName;
            msg += "\")";
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return false;
        }

        {
            static const std::string strLayerName = "Base Layer";

            auto* kAnimLayer = FbxAnimLayer::Create(kScene, strLayerName.c_str());
            if(!kAnimLayer){
                std::string msg = "failed to create FbxAnimLayer";
                msg += "(errored in \"";
                msg += strLayerName;
                msg += "\" of stack \"";
                msg += strStackName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            for(auto* pAnimNode = pAnimStack->AnimationNodes.Values; FBX_PTRDIFFU(pAnimNode - pAnimStack->AnimationNodes.Values) < pAnimStack->AnimationNodes.Length; ++pAnimNode){
                const std::string strNodeName = pAnimNode->BindNode->Name.Values;

                FbxNode* kNode = nullptr;
                {
                    auto f = importNodeToFbxNode.find(pAnimNode->BindNode);
                    if(f == importNodeToFbxNode.cend()){
                        std::string msg = "an error occurred while adding key frame. cannot find bind node";
                        msg += "(errored in \"";
                        msg += strNodeName;
                        msg += "\")";
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }
                    kNode = f->second;
                }

                { // translation
                    auto* kCurveX = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
                    if(!kCurveX){
                        std::string msg = "failed to get curve of X translation component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveY = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    if(!kCurveY){
                        std::string msg = "failed to get curve of Y translation component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveZ = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
                    if(!kCurveZ){
                        std::string msg = "failed to get curve of Z translation component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
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
                        kCurveZ->KeyModifyEnd();
                        kCurveY->KeyModifyEnd();
                        kCurveX->KeyModifyEnd();
                    }
                }

                { // rotation
                    auto* kCurveX = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
                    if(!kCurveX){
                        std::string msg = "failed to get curve of X rotation component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveY = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    if(!kCurveY){
                        std::string msg = "failed to get curve of Y rotation component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveZ = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
                    if(!kCurveZ){
                        std::string msg = "failed to get curve of Z rotation component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
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
                        FbxTime curTime;
                        int curIndex;

                        FbxAMatrix kMat;
                        kMat.SetQ(FbxQuaternion(pKey->Value.Values[0], pKey->Value.Values[1], pKey->Value.Values[2], pKey->Value.Values[3]));
                        auto kVal = kMat.GetR();

                        curTime.SetSecondDouble(pKey->Time);

                        curIndex = kCurveX->KeyAdd(curTime);
                        kCurveX->KeySetValue(curIndex, (float)kVal[0]);
                        kCurveX->KeySetInterpolation(curIndex, curType);

                        curIndex = kCurveY->KeyAdd(curTime);
                        kCurveY->KeySetValue(curIndex, (float)kVal[1]);
                        kCurveY->KeySetInterpolation(curIndex, curType);

                        curIndex = kCurveZ->KeyAdd(curTime);
                        kCurveZ->KeySetValue(curIndex, (float)kVal[2]);
                        kCurveZ->KeySetInterpolation(curIndex, curType);
                    }

                    {
                        kCurveZ->KeyModifyEnd();
                        kCurveY->KeyModifyEnd();
                        kCurveX->KeyModifyEnd();
                    }

                    {
                        FbxAnimCurve* kCurves[] = { kCurveX, kCurveY, kCurveZ };

                        kFilterUnroll.Reset();
                        kFilterUnroll.SetTestForPath(true);
                        if(!kFilterUnroll.Apply((FbxAnimCurve**)&kCurves, _countof(kCurves))){
                            std::string msg = "failed to unroll rotation component";
                            msg += "(errored in \"";
                            msg += strLayerName;
                            msg += "\" of stack \"";
                            msg += strStackName;
                            msg += "\")";
                            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                            return false;
                        }
                    }
                }

                { // scaling
                    auto* kCurveX = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
                    if(!kCurveX){
                        std::string msg = "failed to get curve of X scaling component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveY = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    if(!kCurveY){
                        std::string msg = "failed to get curve of Y scaling component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveZ = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
                    if(!kCurveZ){
                        std::string msg = "failed to get curve of Z scaling component";
                        msg += "(errored in \"";
                        msg += strLayerName;
                        msg += "\" of stack \"";
                        msg += strStackName;
                        msg += "\")";
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
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
                        kCurveZ->KeyModifyEnd();
                        kCurveY->KeyModifyEnd();
                        kCurveX->KeyModifyEnd();
                    }
                }
            }

            if(!kAnimStack->AddMember(kAnimLayer)){
                std::string msg = "failed to add FbxAnimLayer member to FbxAnimStack";
                msg += "(errored in \"";
                msg += strLayerName;
                msg += "\" of stack \"";
                msg += strStackName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
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
