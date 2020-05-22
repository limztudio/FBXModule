/**
* @file FBXShared_Animation.cpp
* @date 2019/04/19
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <set>
#include <unordered_map>

#include <FBXAssign.hpp>

#include "FBXUtilites.h"
#include "FBXMath.h"
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
class _Timestamp{
public:
    inline void clear(){
        for(auto& i : Curves)
            i.clear();

        Markers.clear();
    }


public:
    std::vector<_Marker> Curves[3];
    std::set<FbxTime> Markers;
};


static std::vector<AnimationStack> ins_animationStacks;

static std::set<_Marker> ins_localMarkers;
static _Timestamp ins_localTimestamp;


static inline void ins_updateTimestamp(FbxAnimCurve* kAnimCurve, size_t index){
    if(!kAnimCurve)
        return;

    ins_localMarkers.clear();

    for(auto e = kAnimCurve->KeyGetCount(), i = 0; i < e; ++i){
        _Marker newMaker;
        newMaker.curveKey = kAnimCurve->KeyGet(i);

        ins_localTimestamp.Markers.emplace(newMaker.curveKey.GetTime());
        ins_localMarkers.emplace(std::move(newMaker));
    }

    auto& curves = ins_localTimestamp.Curves[index];
    curves.reserve(ins_localMarkers.size());
    for(auto& i : ins_localMarkers)
        curves.emplace_back(std::move(i));
}
template<typename FUNC>
static inline void ins_fillTimeData(FUNC func){
    static const size_t keyCount = 3;
    size_t lastFrame[keyCount] = { 0, };

    for(const auto& curTime : ins_localTimestamp.Markers){
        FBXAnimationInterpolationType curType[keyCount];
        float curValue[keyCount];

        for(size_t idxCurve = 0; idxCurve < keyCount; ++idxCurve){
            auto& iCurve = ins_localTimestamp.Curves[idxCurve];

            for(size_t idxFrame = lastFrame[idxCurve], edxFrame = iCurve.size(); idxFrame < edxFrame; ++idxFrame){
                auto& kCurve = iCurve[idxFrame].curveKey;
                const auto kTime = kCurve.GetTime();

                if(curTime >= kTime){
                    if(curTime > kTime){
                        switch(kCurve.GetInterpolation()){
                        case FbxAnimCurveDef::eInterpolationConstant:
                            curType[idxCurve] = FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped;
                            curValue[idxCurve] = kCurve.GetValue();
                            break;
                        case FbxAnimCurveDef::eInterpolationLinear:
                        case FbxAnimCurveDef::eInterpolationCubic:
                            curType[idxCurve] = FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear;
                            if((idxFrame + 1) >= edxFrame)
                                curValue[idxCurve] = kCurve.GetValue();
                            else{
                                auto& kNextCurve = iCurve[idxFrame + 1].curveKey;
                                const auto kNextTime = kNextCurve.GetTime();

                                auto dCurTime = curTime.GetSecondDouble();
                                auto dTime = kTime.GetSecondDouble();
                                auto dNextTime = kNextTime.GetSecondDouble();

                                auto dCurPos = (dCurTime - dTime) / (dNextTime - dTime);

                                auto fVal = kCurve.GetValue();
                                auto fNextVal = kNextCurve.GetValue();

                                curValue[idxCurve] = float(fVal + dCurPos * (fNextVal - fVal));
                            }
                            break;
                        }
                    }
                    else
                        curValue[idxCurve] = kCurve.GetValue();

                    lastFrame[idxCurve] = idxFrame;
                    break;
                }
            }
        }

        FBXAnimationInterpolationType genType = FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped;
        for(const auto iType : curType){
            if(iType == FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear){
                genType = iType;
                break;
            }
        }

        func(curTime, genType, curValue);
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
    static const char __name_of_this_func[] = "SHRLoadAnimation(FbxManager*, FbxScene*, const AnimationNodes&)";


    ins_animationStacks.resize((size_t)kScene->GetSrcObjectCount<FbxAnimStack>());
    for(auto edxAnimStack = (int)ins_animationStacks.size(), idxAnimStack = 0; idxAnimStack < edxAnimStack; ++idxAnimStack){
        auto* kAnimStack = kScene->GetSrcObject<FbxAnimStack>(idxAnimStack);
        if(!kAnimStack)
            continue;

        const std::string strStackName = kAnimStack->GetName();

        auto& iAnimStack = ins_animationStacks[idxAnimStack];

        iAnimStack.strName = strStackName.c_str();

        iAnimStack.layers.resize((size_t)kAnimStack->GetMemberCount<FbxAnimLayer>());
        for(auto edxAnimLayer = (int)iAnimStack.layers.size(), idxAnimLayer = 0; idxAnimLayer < edxAnimLayer; ++idxAnimLayer){
            auto* kAnimLayer = kAnimStack->GetMember<FbxAnimLayer>(idxAnimLayer);
            if(!kAnimLayer)
                continue;

            const std::string strLayerName = kAnimLayer->GetName();

            auto& iAnimLayer = iAnimStack.layers[idxAnimLayer];

            iAnimLayer.strName = strLayerName.c_str();
            iAnimLayer.weight = decltype(iAnimLayer.weight)(kAnimLayer->Weight);

            iAnimLayer.nodes.clear();
            iAnimLayer.nodes.reserve(kNodeTable.size());
            for(auto* kNode : kNodeTable){
                AnimationNode newNodes;

                newNodes.bindNode = kNode;

                { // translation
                    ins_localTimestamp.clear();
                    {
                        auto* kCurveX = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
                        ins_updateTimestamp(kCurveX, 0);

                        auto* kCurveY = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
                        ins_updateTimestamp(kCurveY, 1);

                        auto* kCurveZ = kNode->LclTranslation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
                        ins_updateTimestamp(kCurveZ, 2);
                    }

                    {
                        auto& curKeys = newNodes.translationKeys;

                        curKeys.clear();
                        curKeys.reserve(ins_localTimestamp.Markers.size());
                        ins_fillTimeData([&curKeys](const FbxTime& kTime, FBXAnimationInterpolationType iType, const float(&fValue)[3]){
                            FbxDouble3 kVal(fValue[0], fValue[1], fValue[2]);
                            curKeys.emplace_back(kTime, iType, kVal);
                        });
                    }
                }

                { // rotation
                    ins_localTimestamp.clear();
                    {
                        auto* kCurveX = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
                        ins_updateTimestamp(kCurveX, 0);

                        auto* kCurveY = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
                        ins_updateTimestamp(kCurveY, 1);

                        auto* kCurveZ = kNode->LclRotation.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
                        ins_updateTimestamp(kCurveZ, 2);
                    }

                    {
                        auto& curKeys = newNodes.rotationKeys;

                        curKeys.clear();
                        curKeys.reserve(ins_localTimestamp.Markers.size());
                        ins_fillTimeData([&curKeys, &kNode](const FbxTime& kTime, FBXAnimationInterpolationType iType, const float(&fValue)[3]){
                            FbxAMatrix kMat;
                            kMat.SetR(FbxVector4(fValue[0], fValue[1], fValue[2]), kNode->RotationOrder.Get());

                            auto kVal = kMat.GetQ();
                            curKeys.emplace_back(kTime, iType, kVal);
                        });
                    }
                }

                { // scaling
                    ins_localTimestamp.clear();
                    {
                        auto* kCurveX = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
                        ins_updateTimestamp(kCurveX, 0);

                        auto* kCurveY = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
                        ins_updateTimestamp(kCurveY, 1);

                        auto* kCurveZ = kNode->LclScaling.GetCurve(kAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
                        ins_updateTimestamp(kCurveZ, 2);
                    }

                    {
                        auto& curKeys = newNodes.scalingKeys;

                        curKeys.clear();
                        curKeys.reserve(ins_localTimestamp.Markers.size());
                        ins_fillTimeData([&curKeys](const FbxTime& kTime, FBXAnimationInterpolationType iType, const float(&fValue)[3]){
                            FbxDouble3 kVal(fValue[0], fValue[1], fValue[2]);
                            curKeys.emplace_back(kTime, iType, kVal);
                        });
                    }
                }

                if(newNodes.isEmpty()){
                    auto matDefault = GetLocalTransform(kNode);

                    {
                        newNodes.translationKeys.clear();
                        newNodes.translationKeys.reserve(1);
                        auto kValRaw = matDefault.GetT();
                        FbxDouble3 kVal(kValRaw[0], kValRaw[1], kValRaw[2]);
                        newNodes.translationKeys.emplace_back(0, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped, kVal);
                    }
                    {
                        newNodes.rotationKeys.clear();
                        newNodes.rotationKeys.reserve(1);
                        auto kVal = matDefault.GetQ();
                        newNodes.rotationKeys.emplace_back(0, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped, kVal);
                    }
                    {
                        newNodes.scalingKeys.clear();
                        newNodes.scalingKeys.reserve(1);
                        auto kValRaw = matDefault.GetS();
                        FbxDouble3 kVal(kValRaw[0], kValRaw[1], kValRaw[2]);
                        newNodes.scalingKeys.emplace_back(0, FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped, kVal);
                    }

                    iAnimLayer.nodes.emplace_back(std::move(newNodes));
                }
                else
                    iAnimLayer.nodes.emplace_back(std::move(newNodes));
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

        for(auto* pAnimLayer = pAnimStack->AnimationLayers.Values; FBX_PTRDIFFU(pAnimLayer - pAnimStack->AnimationLayers.Values) < pAnimStack->AnimationLayers.Length; ++pAnimLayer){
            const std::string strLayerName = pAnimLayer->Name.Values;

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

            for(auto* pAnimNode = pAnimLayer->AnimationNodes.Values; FBX_PTRDIFFU(pAnimNode - pAnimLayer->AnimationNodes.Values) < pAnimLayer->AnimationNodes.Length; ++pAnimNode){
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
