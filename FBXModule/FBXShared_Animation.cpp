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

        if(iPrevKey.local == kVal.first)
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
                ins_keyCompare(iCur.local, iLhs.local, valDiff) &&
                ins_keyCompare(iCur.local, iRhs.local, valDiff) &&
                ins_keyCompare(iLhs.local, iRhs.local, valDiff)
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

        if(ins_keyCompare(iLhs.local, iRhs.local, valDiff))
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

    CopyArrayData(expKey.Local.Values, fbxKey.local.mData);
    CopyArrayData(expKey.World.Values, fbxKey.world.mData);
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
    //static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRLoadAnimation(FbxManager*, FbxScene*, const AnimationNodes&)");


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
            std::pair<FbxDouble3, FbxDouble3> kDefaultTranslation;
            std::pair<FbxDouble4, FbxDouble4> kDefaultQuaternion;
            std::pair<FbxDouble3, FbxDouble3> kDefaultScaling;
            {
                kDefaultTime.SetSecondDouble(0.);

                {
                    auto kMat = GetLocalTransform(kAnimEvaluator, kNode, kDefaultTime);

                    kDefaultTranslation.first = kMat.GetT();
                    kDefaultQuaternion.first = kMat.GetQ();
                    kDefaultScaling.first = kMat.GetS();
                }
                {
                    auto kMat = GetGlobalTransform(kAnimEvaluator, kNode, kDefaultTime);

                    kDefaultTranslation.second = kMat.GetT();
                    kDefaultQuaternion.second = kMat.GetQ();
                    kDefaultScaling.second = kMat.GetS();
                }
            }

            auto kDefaultMat = GetLocalTransform(kNode);
            AnimationNode newNodes;

            newNodes.bindNode = kNode;

            { // translation
                newNodes.translationKeys.clear();
                newNodes.translationKeys.reserve(ins_animationKeyFrames[0].size());
                for(const auto& kTime : ins_animationKeyFrames[0]){
                    std::pair<FbxDouble3, FbxDouble3> kVal;
                    {
                        auto kMat = GetLocalTransform(kAnimEvaluator, kNode, kTime);
                        kVal.first = kMat.GetT();
                    }
                    {
                        auto kMat = GetGlobalTransform(kAnimEvaluator, kNode, kTime);
                        kVal.second = kMat.GetT();
                    }
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
                    std::pair<FbxDouble4, FbxDouble4> kVal;
                    {
                        auto kMat = GetLocalTransform(kAnimEvaluator, kNode, kTime);
                        kVal.first = kMat.GetQ();
                    }
                    {
                        auto kMat = GetGlobalTransform(kAnimEvaluator, kNode, kTime);
                        kVal.second = kMat.GetQ();
                    }
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
                    std::pair<FbxDouble3, FbxDouble3> kVal;
                    {
                        auto kMat = GetLocalTransform(kAnimEvaluator, kNode, kTime);
                        kVal.first = kMat.GetS();
                    }
                    {
                        auto kMat = GetGlobalTransform(kAnimEvaluator, kNode, kTime);
                        kVal.second = kMat.GetS();
                    }
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
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRLoadAnimations(FbxManager*, FbxScene*, const FbxNodeToExportNode&, FBXDynamicArray<FBXAnimation>*)");


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

        const std::basic_string<FBX_CHAR> strStackName = ConvertString<FBX_CHAR>(iAnimation.animStack->GetName());

        CopyString(pAnimation->Name, strStackName);

        pAnimation->EndTime = decltype(pAnimation->EndTime)(iAnimation.endTime.GetSecondDouble());

        pAnimation->AnimationNodes.Assign(iAnimation.nodes.size());
        for(size_t idxNode = 0; idxNode < pAnimation->AnimationNodes.Length; ++idxNode){
            auto& iNode = iAnimation.nodes[idxNode];
            auto* pNode = &pAnimation->AnimationNodes.Values[idxNode];

            const std::basic_string<FBX_CHAR> strNodeName = ConvertString<FBX_CHAR>(iNode.bindNode->GetName());

            {
                auto f = fbxNodeToExportNode.find(iNode.bindNode);
                if(f == fbxNodeToExportNode.end()){
                    std::basic_string<FBX_CHAR> msg = FBX_TEXT("bind node not found");
                    msg += FBX_TEXT("(errored in \"");
                    msg += strNodeName;
                    msg += FBX_TEXT("\")");
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

                {
                    auto xmm_q = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)&(*pKey).Local.Values);
                    xmm_q = DirectX::XMQuaternionNormalize(xmm_q);
                    DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)&(*pKey).Local.Values, xmm_q);
                }
                {
                    auto xmm_q = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)&(*pKey).World.Values);
                    xmm_q = DirectX::XMQuaternionNormalize(xmm_q);
                    DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)&(*pKey).World.Values, xmm_q);
                }
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
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRStoreAnimation(FbxManager*, FbxScene*, const ImportNodeToFbxNode&, const FBXAnimation*)");


    FbxAnimCurveFilterUnroll kFilterUnroll;
    {
        kFilterUnroll.SetTestForPath(true);
    }

    if(pAnimStack){
        const std::basic_string<FBX_CHAR> strStackName = pAnimStack->Name.Values;

        auto* kAnimStack = FbxAnimStack::Create(kScene, ConvertString<char>(strStackName).c_str());
        if(!kAnimStack){
            std::basic_string<FBX_CHAR> msg = FBX_TEXT("failed to create FbxAnimStack");
            msg += FBX_TEXT("(errored in \"");
            msg += strStackName;
            msg += FBX_TEXT("\")");
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
            static const std::basic_string<FBX_CHAR> strLayerName = FBX_TEXT("Base Layer");

            auto* kAnimLayer = FbxAnimLayer::Create(kScene, ConvertString<char>(strLayerName).c_str());
            if(!kAnimLayer){
                std::basic_string<FBX_CHAR> msg = FBX_TEXT("failed to create FbxAnimLayer");
                msg += FBX_TEXT("(errored in \"");
                msg += strLayerName;
                msg += FBX_TEXT("\" of stack \"");
                msg += strStackName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            for(auto* pAnimNode = pAnimStack->AnimationNodes.Values; FBX_PTRDIFFU(pAnimNode - pAnimStack->AnimationNodes.Values) < pAnimStack->AnimationNodes.Length; ++pAnimNode){
                const std::basic_string<FBX_CHAR> strNodeName = pAnimNode->BindNode->Name.Values;

                FbxNode* kNode = nullptr;
                {
                    auto f = importNodeToFbxNode.find(pAnimNode->BindNode);
                    if(f == importNodeToFbxNode.cend()){
                        std::basic_string<FBX_CHAR> msg = FBX_TEXT("an error occurred while adding key frame. cannot find bind node");
                        msg += FBX_TEXT("(errored in \"");
                        msg += strNodeName;
                        msg += FBX_TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }
                    kNode = f->second;
                }

                if(pAnimNode->TranslationKeys.Length){ // translation
                    auto* kCurveX = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
                    if(!kCurveX){
                        std::basic_string<FBX_CHAR> msg = FBX_TEXT("failed to get curve of X translation component");
                        msg += FBX_TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += FBX_TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += FBX_TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveY = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    if(!kCurveY){
                        std::basic_string<FBX_CHAR> msg = FBX_TEXT("failed to get curve of Y translation component");
                        msg += FBX_TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += FBX_TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += FBX_TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveZ = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
                    if(!kCurveZ){
                        std::basic_string<FBX_CHAR> msg = FBX_TEXT("failed to get curve of Z translation component");
                        msg += FBX_TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += FBX_TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += FBX_TEXT("\")");
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
                        kCurveX->KeySetValue(curIndex, pKey->Local.Values[0]);
                        kCurveX->KeySetInterpolation(curIndex, curType);

                        curIndex = kCurveY->KeyAdd(curTime);
                        kCurveY->KeySetValue(curIndex, pKey->Local.Values[1]);
                        kCurveY->KeySetInterpolation(curIndex, curType);

                        curIndex = kCurveZ->KeyAdd(curTime);
                        kCurveZ->KeySetValue(curIndex, pKey->Local.Values[2]);
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
                        std::basic_string<FBX_CHAR> msg = FBX_TEXT("failed to get curve of X rotation component");
                        msg += FBX_TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += FBX_TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += FBX_TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveY = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    if(!kCurveY){
                        std::basic_string<FBX_CHAR> msg = FBX_TEXT("failed to get curve of Y rotation component");
                        msg += FBX_TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += FBX_TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += FBX_TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveZ = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
                    if(!kCurveZ){
                        std::basic_string<FBX_CHAR> msg = FBX_TEXT("failed to get curve of Z rotation component");
                        msg += FBX_TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += FBX_TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += FBX_TEXT("\")");
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
                        kMat.SetQ(FbxQuaternion(pKey->Local.Values[0], pKey->Local.Values[1], pKey->Local.Values[2], pKey->Local.Values[3]));
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
                            std::basic_string<FBX_CHAR> msg = FBX_TEXT("failed to unroll rotation component");
                            msg += FBX_TEXT("(errored in \"");
                            msg += strLayerName;
                            msg += FBX_TEXT("\" of stack \"");
                            msg += strStackName;
                            msg += FBX_TEXT("\")");
                            SHRPushWarningMessage(std::move(msg), __name_of_this_func);
                        }
                    }
                }

                if(pAnimNode->ScalingKeys.Length){ // scaling
                    auto* kCurveX = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
                    if(!kCurveX){
                        std::basic_string<FBX_CHAR> msg = FBX_TEXT("failed to get curve of X scaling component");
                        msg += FBX_TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += FBX_TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += FBX_TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveY = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    if(!kCurveY){
                        std::basic_string<FBX_CHAR> msg = FBX_TEXT("failed to get curve of Y scaling component");
                        msg += FBX_TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += FBX_TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += FBX_TEXT("\")");
                        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                        return false;
                    }

                    auto* kCurveZ = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
                    if(!kCurveZ){
                        std::basic_string<FBX_CHAR> msg = FBX_TEXT("failed to get curve of Z scaling component");
                        msg += FBX_TEXT("(errored in \"");
                        msg += strLayerName;
                        msg += FBX_TEXT("\" of stack \"");
                        msg += strStackName;
                        msg += FBX_TEXT("\")");
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
                        kCurveX->KeySetValue(curIndex, pKey->Local.Values[0]);
                        kCurveX->KeySetInterpolation(curIndex, curType);

                        curIndex = kCurveY->KeyAdd(curTime);
                        kCurveY->KeySetValue(curIndex, pKey->Local.Values[1]);
                        kCurveY->KeySetInterpolation(curIndex, curType);

                        curIndex = kCurveZ->KeyAdd(curTime);
                        kCurveZ->KeySetValue(curIndex, pKey->Local.Values[2]);
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
                std::basic_string<FBX_CHAR> msg = FBX_TEXT("failed to add FbxAnimLayer member to FbxAnimStack");
                msg += FBX_TEXT("(errored in \"");
                msg += strLayerName;
                msg += FBX_TEXT("\" of stack \"");
                msg += strStackName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }
        }
    }

    return true;
}
bool SHRStoreAnimations(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const ImportNodeToFbxNode& importNodeToFbxNode, const FBXDynamicArray<FBXAnimation>& animStacks){
    //static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRStoreAnimations(FbxManager*, FbxScene*, const ImportNodeToFbxNode&, const FBXDynamicArray<FBXAnimation>&)");


    for(size_t idxAnimation = 0; idxAnimation < animStacks.Length; ++idxAnimation){
        const auto* pAnimStack = &animStacks.Values[idxAnimation];

        if(!SHRStoreAnimation(kSDKManager, kScene, importNodeToFbxNode, pAnimStack))
            return false;
    }

    return true;
}
