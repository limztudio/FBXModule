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

template<typename KEY, typename DIFF, size_t KEY_COUNT = _countof(KEY::mData)>
static inline bool ins_keyCompare(const KEY& lhs, const KEY& rhs, DIFF valDiff){
    for(size_t i = 0; i < KEY_COUNT; ++i){
        if(std::abs((DIFF)lhs.mData[i] - (DIFF)rhs.mData[i]) > valDiff)
            return false;
    }
    return true;
}
template<typename KEY, typename DIFF, size_t KEY_COUNT = _countof(KEY::mData)>
static inline void ins_keyReduce(AnimationKeyFrames<KEY>& keyTable, DIFF valDiff){
    if(keyTable.size() > 2){
        size_t idxPivot = 1;
        while((keyTable.size() > 2) && ((idxPivot + 1) < keyTable.size())){
            const auto& iLhs = keyTable[idxPivot - 1];
            const auto& iCur = keyTable[idxPivot];
            const auto& iRhs = keyTable[idxPivot + 1];

            if(
                ins_keyCompare(iCur.value, iLhs.value, valDiff) &&
                ins_keyCompare(iCur.value, iRhs.value, valDiff) &&
                ins_keyCompare(iLhs.value, iRhs.value, valDiff)
                )
            {
                auto itr = keyTable.begin();
                std::advance(itr, idxPivot);

                keyTable.erase(itr);
                continue;
            }

            ++idxPivot;
        }
    }

    if(keyTable.size() == 2){
        const auto& iLhs = keyTable[0];
        const auto& iRhs = keyTable[1];

        if(ins_keyCompare(iLhs.value, iRhs.value, valDiff))
            keyTable.pop_back();
    }
}

static inline void ins_updateTimestamp(const FbxTime& kEndTime, FbxAnimCurve* kAnimCurve, size_t idx){
    if(!kAnimCurve)
        return;

    for(auto e = kAnimCurve->KeyGetCount(), i = 0; i < e; ++i){
        auto kCurTime = kAnimCurve->KeyGet(i).GetTime();
        if(kCurTime <= kEndTime)
            ins_animationKeyFrames[idx].emplace(kCurTime);
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
    //static const TCHAR __name_of_this_func[] = TEXT("SHRLoadAnimation(FbxManager*, FbxScene*, const AnimationNodes&)");


    auto* kDefaultAnimStack = kScene->GetCurrentAnimationStack();

    auto* kAnimEvaluator = kScene->GetAnimationEvaluator();

    const auto edxAnimStack = kScene->GetSrcObjectCount<FbxAnimStack>();

    ins_animationStacks.clear();
    ins_animationStacks.reserve((size_t)edxAnimStack);

    for(auto idxAnimStack = decltype(edxAnimStack){ 0 }; idxAnimStack < edxAnimStack; ++idxAnimStack){
        auto* kAnimStack = kScene->GetSrcObject<FbxAnimStack>(idxAnimStack);
        if(!kAnimStack)
            continue;

        kScene->SetCurrentAnimationStack(kAnimStack);

        auto kTimeSpan = kAnimStack->GetLocalTimeSpan();

        ins_animationStacks.emplace_back(AnimationStack());
        auto& iAnimStack = *ins_animationStacks.rbegin();

        iAnimStack.animStack = kAnimStack;
        iAnimStack.endTime = kTimeSpan.GetStop();

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
                    ins_updateTimestamp(iAnimStack.endTime, kCurveX, 0);

                    auto* kCurveY = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
                    ins_updateTimestamp(iAnimStack.endTime, kCurveY, 0);

                    auto* kCurveZ = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
                    ins_updateTimestamp(iAnimStack.endTime, kCurveZ, 0);
                }

                { // rotation
                    auto* kCurveX = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
                    ins_updateTimestamp(iAnimStack.endTime, kCurveX, 1);

                    auto* kCurveY = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
                    ins_updateTimestamp(iAnimStack.endTime, kCurveY, 1);

                    auto* kCurveZ = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
                    ins_updateTimestamp(iAnimStack.endTime, kCurveZ, 1);
                }

                { // scaling
                    auto* kCurveX = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
                    ins_updateTimestamp(iAnimStack.endTime, kCurveX, 2);

                    auto* kCurveY = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
                    ins_updateTimestamp(iAnimStack.endTime, kCurveY, 2);

                    auto* kCurveZ = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
                    ins_updateTimestamp(iAnimStack.endTime, kCurveZ, 2);
                }
            }

            FbxTime kDefaultTime;
            FbxDouble3 kDefaultTranslation;
            FbxDouble4 kDefaultQuaternion;
            FbxDouble3 kDefaultScaling;
            {
                kDefaultTime.SetSecondDouble(0.);

                auto kMat = GetLocalTransform(kAnimEvaluator, kNode, kDefaultTime);

                kDefaultTranslation = kMat.GetT();
                kDefaultQuaternion = kMat.GetQ();
                kDefaultScaling = kMat.GetS();
            }

            auto kDefaultMat = GetLocalTransform(kNode);
            AnimationNode newNodes;

            newNodes.bindNode = kNode;

            { // translation
                newNodes.translationKeys.clear();
                newNodes.translationKeys.reserve(ins_animationKeyFrames[0].size());
                for(const auto& kTime : ins_animationKeyFrames[0]){
                    auto kMat = GetLocalTransform(kAnimEvaluator, kNode, kTime);
                    FbxDouble3 kVal = kMat.GetT();
                    newNodes.translationKeys.emplace_back(kTime, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear, kVal);
                    ins_keyTypeOptimze(newNodes.translationKeys, kVal);
                }

                ins_keyReduce(newNodes.translationKeys, shr_ioSetting.AnimationKeyCompareDifference);

                if(newNodes.translationKeys.empty()){
                    newNodes.translationKeys.reserve(1);
                    newNodes.translationKeys.emplace_back(kDefaultTime, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped, kDefaultTranslation);
                }
            }

            { // rotation
                newNodes.rotationKeys.clear();
                newNodes.rotationKeys.reserve(ins_animationKeyFrames[1].size());
                for(const auto& kTime : ins_animationKeyFrames[1]){
                    auto kMat = GetLocalTransform(kAnimEvaluator, kNode, kTime);
                    FbxDouble4 kVal = kMat.GetQ();
                    newNodes.rotationKeys.emplace_back(kTime, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear, kVal);
                    ins_keyTypeOptimze(newNodes.rotationKeys, kVal);
                }

                ins_keyReduce(newNodes.rotationKeys, shr_ioSetting.AnimationKeyCompareDifference);

                if(newNodes.rotationKeys.empty()){
                    newNodes.rotationKeys.reserve(1);
                    newNodes.rotationKeys.emplace_back(kDefaultTime, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped, kDefaultQuaternion);
                }
            }

            { // scaling
                newNodes.scalingKeys.clear();
                newNodes.scalingKeys.reserve(ins_animationKeyFrames[2].size());
                for(const auto& kTime : ins_animationKeyFrames[2]){
                    auto kMat = GetLocalTransform(kAnimEvaluator, kNode, kTime);
                    FbxDouble3 kVal = kMat.GetS();
                    newNodes.scalingKeys.emplace_back(kTime, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear, kVal);
                    ins_keyTypeOptimze(newNodes.scalingKeys, kVal);
                }

                ins_keyReduce(newNodes.scalingKeys, shr_ioSetting.AnimationKeyCompareDifference);

                if(newNodes.scalingKeys.empty()){
                    newNodes.scalingKeys.reserve(1);
                    newNodes.scalingKeys.emplace_back(kDefaultTime, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped, kDefaultScaling);
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

    kScene->SetCurrentAnimationStack(kDefaultAnimStack);

    return true;
}
bool SHRLoadAnimations(FbxManager* kSDKManager, FbxScene* kScene, const FbxNodeToExportNode& fbxNodeToExportNode, FBXDynamicArray<FBXAnimation>* pAnimations){
    static const TCHAR __name_of_this_func[] = TEXT("SHRLoadAnimations(FbxManager*, FbxScene*, const FbxNodeToExportNode&, FBXDynamicArray<FBXAnimation>*)");


    AnimationNodes kNodeTable;
    {
        kNodeTable.reserve(fbxNodeToExportNode.size());
        ins_collectNodes(kNodeTable, kScene->GetRootNode());
    }

    ins_animationStacks.clear();
    SHRLoadAnimation(kSDKManager, kScene, kNodeTable);

    pAnimations->Assign(ins_animationStacks.size());
    for(size_t idxAnimation = 0; idxAnimation < pAnimations->Length; ++idxAnimation){
        auto& iAnimation = ins_animationStacks[idxAnimation];
        auto* pAnimation = &pAnimations->Values[idxAnimation];

        const std::basic_string<TCHAR> strStackName = ConvertString<TCHAR>(iAnimation.animStack->GetName());

        CopyString(pAnimation->Name, strStackName);

        pAnimation->EndTime = decltype(pAnimation->EndTime)(iAnimation.endTime.GetSecondDouble());

        pAnimation->AnimationNodes.Assign(iAnimation.nodes.size());
        for(size_t idxNode = 0; idxNode < pAnimation->AnimationNodes.Length; ++idxNode){
            auto& iNode = iAnimation.nodes[idxNode];
            auto* pNode = &pAnimation->AnimationNodes.Values[idxNode];

            const std::basic_string<TCHAR> strNodeName = ConvertString<TCHAR>(iNode.bindNode->GetName());

            {
                auto f = fbxNodeToExportNode.find(iNode.bindNode);
                if(f == fbxNodeToExportNode.end()){
                    std::basic_string<TCHAR> msg = TEXT("bind node not found");
                    msg += TEXT("(errored in \"");
                    msg += strNodeName;
                    msg += TEXT("\")");
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

                auto xmm_q = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)&(*pKey).Value.Values);
                xmm_q = DirectX::XMQuaternionNormalize(xmm_q);
                DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)&(*pKey).Value.Values, xmm_q);
            }

            pNode->TranslationKeys.Assign(iNode.translationKeys.size());
            for(size_t idxKey = 0; idxKey < pNode->TranslationKeys.Length; ++idxKey){
                auto& iKey = iNode.translationKeys[idxKey];
                auto* pKey = &pNode->TranslationKeys.Values[idxKey];

                ins_convAnimationKey(*pKey, iKey);
            }
        }
    }

    return true;
}

bool SHRStoreAnimation(FbxManager* kSDKManager, FbxScene* kScene, const ImportNodeToFbxNode& importNodeToFbxNode, const FBXAnimation* pAnimStack){
    static const TCHAR __name_of_this_func[] = TEXT("SHRStoreAnimation(FbxManager*, FbxScene*, const ImportNodeToFbxNode&, const FBXAnimation*)");


    FbxAnimCurveFilterUnroll kFilterUnroll;
    {
        kFilterUnroll.SetTestForPath(true);
    }

    if(pAnimStack){
        const std::basic_string<TCHAR> strStackName = pAnimStack->Name.Values;

        auto* kAnimStack = FbxAnimStack::Create(kScene, ConvertString<char>(strStackName).c_str());
        if(!kAnimStack){
            std::basic_string<TCHAR> msg = TEXT("failed to create FbxAnimStack");
            msg += TEXT("(errored in \"");
            msg += strStackName;
            msg += TEXT("\")");
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return false;
        }

        {
            FbxTime kTime;
            FbxTimeSpan kTimeSpan;

            kTime.SetSecondDouble(0.);
            kTimeSpan.SetStart(kTime);

            kTime.SetSecondDouble(pAnimStack->EndTime);
            kTimeSpan.SetStop(kTime);

            kAnimStack->SetLocalTimeSpan(kTimeSpan);
        }

        {
            static const std::basic_string<TCHAR> strLayerName = TEXT("Base Layer");

            auto* kAnimLayer = FbxAnimLayer::Create(kScene, ConvertString<char>(strLayerName).c_str());
            if(!kAnimLayer){
                std::basic_string<TCHAR> msg = TEXT("failed to create FbxAnimLayer");
                msg += TEXT("(errored in \"");
                msg += strLayerName;
                msg += TEXT("\" of stack \"");
                msg += strStackName;
                msg += TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            for(auto* pAnimNode = pAnimStack->AnimationNodes.Values; FBX_PTRDIFFU(pAnimNode - pAnimStack->AnimationNodes.Values) < pAnimStack->AnimationNodes.Length; ++pAnimNode){
                const std::basic_string<TCHAR> strNodeName = pAnimNode->BindNode->Name.Values;

                FbxNode* kNode = nullptr;
                {
                    auto f = importNodeToFbxNode.find(pAnimNode->BindNode);
                    if(f == importNodeToFbxNode.cend()){
                        std::basic_string<TCHAR> msg = TEXT("an error occurred while adding key frame. cannot find bind node");
                        msg += TEXT("(errored in \"");
                        msg += strNodeName;
                        msg += TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }
                    kNode = f->second;
                }

                if(pAnimNode->TranslationKeys.Length){ // translation
                    auto* kCurveX = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
                    if(!kCurveX){
                        std::basic_string<TCHAR> msg = TEXT("failed to get curve of X translation component");
                        msg += TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveY = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    if(!kCurveY){
                        std::basic_string<TCHAR> msg = TEXT("failed to get curve of Y translation component");
                        msg += TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveZ = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
                    if(!kCurveZ){
                        std::basic_string<TCHAR> msg = TEXT("failed to get curve of Z translation component");
                        msg += TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += TEXT("\")");
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

                if(pAnimNode->RotationKeys.Length){ // rotation
                    auto* kCurveX = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
                    if(!kCurveX){
                        std::basic_string<TCHAR> msg = TEXT("failed to get curve of X rotation component");
                        msg += TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveY = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    if(!kCurveY){
                        std::basic_string<TCHAR> msg = TEXT("failed to get curve of Y rotation component");
                        msg += TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveZ = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
                    if(!kCurveZ){
                        std::basic_string<TCHAR> msg = TEXT("failed to get curve of Z rotation component");
                        msg += TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += TEXT("\")");
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

                        if(!kFilterUnroll.Apply((FbxAnimCurve**)&kCurves, _countof(kCurves))){
                            std::basic_string<TCHAR> msg = TEXT("failed to unroll rotation component");
                            msg += TEXT("(errored in \"");
                            msg += strLayerName;
                            msg += TEXT("\" of stack \"");
                            msg += strStackName;
                            msg += TEXT("\")");
                            SHRPushWarningMessage(std::move(msg), __name_of_this_func);
                        }
                    }
                }

                if(pAnimNode->ScalingKeys.Length){ // scaling
                    auto* kCurveX = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
                    if(!kCurveX){
                        std::basic_string<TCHAR> msg = TEXT("failed to get curve of X scaling component");
                        msg += TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveY = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    if(!kCurveY){
                        std::basic_string<TCHAR> msg = TEXT("failed to get curve of Y scaling component");
                        msg += TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveZ = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
                    if(!kCurveZ){
                        std::basic_string<TCHAR> msg = TEXT("failed to get curve of Z scaling component");
                        msg += TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += TEXT("\")");
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
                std::basic_string<TCHAR> msg = TEXT("failed to add FbxAnimLayer member to FbxAnimStack");
                msg += TEXT("(errored in \"");
                msg += strLayerName;
                msg += TEXT("\" of stack \"");
                msg += strStackName;
                msg += TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }
        }
    }

    return true;
}
bool SHRStoreAnimations(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const ImportNodeToFbxNode& importNodeToFbxNode, const FBXDynamicArray<FBXAnimation>& animStacks){
    //static const TCHAR __name_of_this_func[] = TEXT("SHRStoreAnimations(FbxManager*, FbxScene*, const ImportNodeToFbxNode&, const FBXDynamicArray<FBXAnimation>&)");


    for(size_t idxAnimation = 0; idxAnimation < animStacks.Length; ++idxAnimation){
        const auto* pAnimStack = &animStacks.Values[idxAnimation];

        if(!SHRStoreAnimation(kSDKManager, kScene, importNodeToFbxNode, pAnimStack))
            return false;
    }

    return true;
}
