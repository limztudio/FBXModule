/**
* @file FBXModule_Utilites.cpp
* @date 2020/06/05
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <algorithm>
#include <unordered_map>

#include "FBXMath.h"
#include "FBXShared.h"


template<typename T, unsigned long N>
static inline void ins_interpolateValue(T(&pOut)[N], const FBXStaticArray<T, N>* pV0, const FBXStaticArray<T, N>* pV1, float t){}
template<>
inline void ins_interpolateValue(float(&pOut)[3], const FBXStaticArray<float, 3>* pV0, const FBXStaticArray<float, 3>* pV1, float t){
    auto xmm_v0 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)pV0->Values);
    auto xmm_v1 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)pV1->Values);

    auto xmm_v = DirectX::XMVectorLerp(xmm_v0, xmm_v1, t);
    DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)pOut, xmm_v);
}
template<>
inline void ins_interpolateValue(float(&pOut)[4], const FBXStaticArray<float, 4>* pV0, const FBXStaticArray<float, 4>* pV1, float t){
    auto xmm_v0 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)pV0->Values);
    auto xmm_v1 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)pV1->Values);

    auto xmm_v = DirectX::XMQuaternionSlerp(xmm_v0, xmm_v1, t);
    xmm_v = DirectX::XMQuaternionNormalize(xmm_v);
    DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)pOut, xmm_v);
}

template<typename T, unsigned long N>
static inline void ins_computeLocalByTime(T(&pOut)[N], float time, const FBXDynamicArray<FBXAnimationKeyFrame<FBXStaticArray<T, N>>>* pTable){
    const FBXAnimationKeyFrame<FBXStaticArray<T, N>>* pNextData = pTable->Values;
    const FBXAnimationKeyFrame<FBXStaticArray<T, N>>* pData = pNextData++;
    for(; FBX_PTRDIFFU(pNextData - pTable->Values) < pTable->Length; pData = pNextData++){
        if(pNextData->Time > time){
            switch(pData->InterpolationType){
            case FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped:
            {
                CopyArrayData(pOut, pData->Local.Values);
                return;
            }
            case FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear:
            {
                auto fTime = (time - pData->Time) / (pNextData->Time - pData->Time);
                ins_interpolateValue<T, N>(pOut, &pData->Local, &pNextData->Local, fTime);
                return;
            }
            }
        }
    }

    CopyArrayData(pOut, pData->Local.Values);
}
template<typename T, unsigned long N>
static inline void ins_computeWorldByTime(T(&pOut)[N], float time, const FBXDynamicArray<FBXAnimationKeyFrame<FBXStaticArray<T, N>>>* pTable){
    const FBXAnimationKeyFrame<FBXStaticArray<T, N>>* pNextData = pTable->Values;
    const FBXAnimationKeyFrame<FBXStaticArray<T, N>>* pData = pNextData++;
    for(; FBX_PTRDIFFU(pNextData - pTable->Values) < pTable->Length; pData = pNextData++){
        if(pNextData->Time > time){
            switch(pData->InterpolationType){
            case FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped:
            {
                CopyArrayData(pOut, pData->World.Values);
                return;
            }
            case FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear:
            {
                auto fTime = (time - pData->Time) / (pNextData->Time - pData->Time);
                ins_interpolateValue<T, N>(pOut, &pData->World, &pNextData->World, fTime);
                return;
            }
            }
        }
    }

    CopyArrayData(pOut, pData->World.Values);
}


__FBXM_MAKE_FUNC(void, FBXGetWorldMatrix, void* pOutMatrix, const void* pNode){
    const auto* pConvNode = reinterpret_cast<const FBXNode*>(pNode);

    //FbxAMatrix kMatRes;
    //CopyArrayData<16>((double*)kMatRes, pConvNode->TransformMatrix.Values);
    //for(pConvNode = pConvNode->Parent; pConvNode; pConvNode = pConvNode->Parent){
    //    FbxAMatrix kMatTmp;
    //    CopyArrayData<16>((double*)kMatTmp, pConvNode->TransformMatrix.Values);
    //    kMatRes = kMatTmp * kMatRes;
    //}
    //CopyArrayData<16>((float*)pOutMatrix, (double*)kMatRes);

    auto xmm4_ret = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pConvNode->TransformMatrix.Values);
    for(pConvNode = pConvNode->Parent; pConvNode; pConvNode = pConvNode->Parent){
        auto xmm4_tmp = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pConvNode->TransformMatrix.Values);
        xmm4_ret = DirectX::XMMatrixMultiply(xmm4_ret, xmm4_tmp);
    }
    DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)pOutMatrix, xmm4_ret);
}
__FBXM_MAKE_FUNC(void, FBXTransformCoord, void* pOutVec3, const void* pVec3, const void* pMatrix){
    auto xmm4_srt = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pMatrix);
    auto xmm_vec = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)pVec3);

    auto xmm_ret = DirectX::XMVector3TransformCoord(xmm_vec, xmm4_srt);

    DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)pOutVec3, xmm_ret);
}
__FBXM_MAKE_FUNC(void, FBXTransformNormal, void* pOutVec3, const void* pVec3, const void* pMatrix){
    auto xmm4_srt = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pMatrix);
    auto xmm_vec = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)pVec3);

    auto xmm_ret = DirectX::XMVector3TransformNormal(xmm_vec, xmm4_srt);
    xmm_ret = DirectX::XMVector3Normalize(xmm_ret);

    DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)pOutVec3, xmm_ret);
}


__FBXM_MAKE_FUNC(void, FBXComputeAnimationLocalTransform, void* pOutScale, void* pOutRotation, void* pOutTranslation, const void* pAnimationNode, float time){
    const auto* pConvAnimationNode = reinterpret_cast<const FBXAnimationNode*>(pAnimationNode);

    Float3* pConvOutScale = reinterpret_cast<decltype(pConvOutScale)>(pOutScale);
    Float4* pConvOutRotation = reinterpret_cast<decltype(pConvOutRotation)>(pOutRotation);
    Float3* pConvOutTranslation = reinterpret_cast<decltype(pConvOutTranslation)>(pOutTranslation);

    FbxAMatrix matRef;
    CopyArrayData<pConvAnimationNode->BindNode->TransformMatrix.Length>((double*)matRef, pConvAnimationNode->BindNode->TransformMatrix.Values);

    if(!pConvAnimationNode->ScalingKeys.Length){
        auto vValue = matRef.GetS();
        CopyArrayData(pConvOutScale->raw, vValue.mData);
    }
    else{
        auto fTime = std::min(time, pConvAnimationNode->ScalingKeys.Values[pConvAnimationNode->ScalingKeys.Length - 1].Time);
        ins_computeLocalByTime(pConvOutScale->raw, fTime, &pConvAnimationNode->ScalingKeys);
    }

    if(!pConvAnimationNode->RotationKeys.Length){
        auto vValue = matRef.GetQ();
        CopyArrayData(pConvOutRotation->raw, vValue.mData);
    }
    else{
        auto fTime = std::min(time, pConvAnimationNode->RotationKeys.Values[pConvAnimationNode->RotationKeys.Length - 1].Time);
        ins_computeLocalByTime(pConvOutRotation->raw, fTime, &pConvAnimationNode->RotationKeys);
    }

    if(!pConvAnimationNode->TranslationKeys.Length){
        auto vValue = matRef.GetT();
        CopyArrayData(pConvOutTranslation->raw, vValue.mData);
    }
    else{
        auto fTime = std::min(time, pConvAnimationNode->TranslationKeys.Values[pConvAnimationNode->TranslationKeys.Length - 1].Time);
        ins_computeLocalByTime(pConvOutTranslation->raw, fTime, &pConvAnimationNode->TranslationKeys);
    }
}
__FBXM_MAKE_FUNC(void, FBXComputeAnimationWorldTransform, void* pOutScale, void* pOutRotation, void* pOutTranslation, const void* pAnimationNode, float time){
    const auto* pConvAnimationNode = reinterpret_cast<const FBXAnimationNode*>(pAnimationNode);

    Float3* pConvOutScale = reinterpret_cast<decltype(pConvOutScale)>(pOutScale);
    Float4* pConvOutRotation = reinterpret_cast<decltype(pConvOutRotation)>(pOutRotation);
    Float3* pConvOutTranslation = reinterpret_cast<decltype(pConvOutTranslation)>(pOutTranslation);

    FbxAMatrix matRef;
    {
        auto xmm4_ret = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pConvAnimationNode->BindNode->TransformMatrix.Values);
        for(auto* pNode = pConvAnimationNode->BindNode->Parent; pNode; pNode = pNode->Parent){
            auto xmm4_tmp = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pNode->TransformMatrix.Values);
            xmm4_ret = DirectX::XMMatrixMultiply(xmm4_ret, xmm4_tmp);
        }
        DirectX::XMFLOAT4X4A matFlt;
        DirectX::XMStoreFloat4x4A(&matFlt, xmm4_ret);
        CopyArrayData<pConvAnimationNode->BindNode->TransformMatrix.Length>((double*)matRef, (float*)&matFlt);
    }

    if(!pConvAnimationNode->ScalingKeys.Length) {
        auto vValue = matRef.GetS();
        CopyArrayData(pConvOutScale->raw, vValue.mData);
    }
    else{
        auto fTime = std::min(time, pConvAnimationNode->ScalingKeys.Values[pConvAnimationNode->ScalingKeys.Length - 1].Time);
        ins_computeWorldByTime(pConvOutScale->raw, fTime, &pConvAnimationNode->ScalingKeys);
    }

    if(!pConvAnimationNode->RotationKeys.Length) {
        auto vValue = matRef.GetQ();
        CopyArrayData(pConvOutRotation->raw, vValue.mData);
    }
    else{
        auto fTime = std::min(time, pConvAnimationNode->RotationKeys.Values[pConvAnimationNode->RotationKeys.Length - 1].Time);
        ins_computeWorldByTime(pConvOutRotation->raw, fTime, &pConvAnimationNode->RotationKeys);
    }

    if(!pConvAnimationNode->TranslationKeys.Length) {
        auto vValue = matRef.GetT();
        CopyArrayData(pConvOutTranslation->raw, vValue.mData);
    }
    else{
        auto fTime = std::min(time, pConvAnimationNode->TranslationKeys.Values[pConvAnimationNode->TranslationKeys.Length - 1].Time);
        ins_computeWorldByTime(pConvOutTranslation->raw, fTime, &pConvAnimationNode->TranslationKeys);
    }
}

__FBXM_MAKE_FUNC(void, FBXComputeAnimationLocalScale, void* pOutScale, const void* pAnimationNode, float time){
    const auto* pConvAnimationNode = reinterpret_cast<const FBXAnimationNode*>(pAnimationNode);

    Float3* pConvOutScale = reinterpret_cast<decltype(pConvOutScale)>(pOutScale);

    if(!pConvAnimationNode->ScalingKeys.Length){
        FbxAMatrix matRef;
        CopyArrayData<pConvAnimationNode->BindNode->TransformMatrix.Length>((double*)matRef, pConvAnimationNode->BindNode->TransformMatrix.Values);

        auto vValue = matRef.GetS();
        CopyArrayData(pConvOutScale->raw, vValue.mData);
    }
    else{
        auto fTime = std::clamp(
            time,
            pConvAnimationNode->ScalingKeys.Values[0].Time,
            pConvAnimationNode->ScalingKeys.Values[pConvAnimationNode->ScalingKeys.Length - 1].Time
        );
        ins_computeLocalByTime(pConvOutScale->raw, fTime, &pConvAnimationNode->ScalingKeys);
    }
}
__FBXM_MAKE_FUNC(void, FBXComputeAnimationWorldScale, void* pOutScale, const void* pAnimationNode, float time){
    const auto* pConvAnimationNode = reinterpret_cast<const FBXAnimationNode*>(pAnimationNode);

    Float3* pConvOutScale = reinterpret_cast<decltype(pConvOutScale)>(pOutScale);

    if(!pConvAnimationNode->ScalingKeys.Length){
        FbxAMatrix matRef;
        {
            auto xmm4_ret = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pConvAnimationNode->BindNode->TransformMatrix.Values);
            for(auto* pNode = pConvAnimationNode->BindNode->Parent; pNode; pNode = pNode->Parent){
                auto xmm4_tmp = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pNode->TransformMatrix.Values);
                xmm4_ret = DirectX::XMMatrixMultiply(xmm4_ret, xmm4_tmp);
            }
            DirectX::XMFLOAT4X4A matFlt;
            DirectX::XMStoreFloat4x4A(&matFlt, xmm4_ret);
            CopyArrayData<pConvAnimationNode->BindNode->TransformMatrix.Length>((double*)matRef, (float*)&matFlt);
        }

        auto vValue = matRef.GetS();
        CopyArrayData(pConvOutScale->raw, vValue.mData);
    }
    else{
        auto fTime = std::clamp(
            time,
            pConvAnimationNode->ScalingKeys.Values[0].Time,
            pConvAnimationNode->ScalingKeys.Values[pConvAnimationNode->ScalingKeys.Length - 1].Time
        );
        ins_computeWorldByTime(pConvOutScale->raw, fTime, &pConvAnimationNode->ScalingKeys);
    }
}

__FBXM_MAKE_FUNC(void, FBXComputeAnimationLocalRotation, void* pOutRotation, const void* pAnimationNode, float time){
    const auto* pConvAnimationNode = reinterpret_cast<const FBXAnimationNode*>(pAnimationNode);

    Float4* pConvOutRotation = reinterpret_cast<decltype(pConvOutRotation)>(pOutRotation);

    if(!pConvAnimationNode->RotationKeys.Length){
        FbxAMatrix matRef;
        CopyArrayData<pConvAnimationNode->BindNode->TransformMatrix.Length>((double*)matRef, pConvAnimationNode->BindNode->TransformMatrix.Values);

        auto vValue = matRef.GetQ();
        CopyArrayData(pConvOutRotation->raw, vValue.mData);
    }
    else{
        auto fTime = std::clamp(
            time,
            pConvAnimationNode->RotationKeys.Values[0].Time,
            pConvAnimationNode->RotationKeys.Values[pConvAnimationNode->RotationKeys.Length - 1].Time
        );
        ins_computeLocalByTime(pConvOutRotation->raw, fTime, &pConvAnimationNode->RotationKeys);
    }
}
__FBXM_MAKE_FUNC(void, FBXComputeAnimationWorldRotation, void* pOutRotation, const void* pAnimationNode, float time){
    const auto* pConvAnimationNode = reinterpret_cast<const FBXAnimationNode*>(pAnimationNode);

    Float4* pConvOutRotation = reinterpret_cast<decltype(pConvOutRotation)>(pOutRotation);

    if(!pConvAnimationNode->RotationKeys.Length){
        FbxAMatrix matRef;
        {
            auto xmm4_ret = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pConvAnimationNode->BindNode->TransformMatrix.Values);
            for(auto* pNode = pConvAnimationNode->BindNode->Parent; pNode; pNode = pNode->Parent){
                auto xmm4_tmp = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pNode->TransformMatrix.Values);
                xmm4_ret = DirectX::XMMatrixMultiply(xmm4_ret, xmm4_tmp);
            }
            DirectX::XMFLOAT4X4A matFlt;
            DirectX::XMStoreFloat4x4A(&matFlt, xmm4_ret);
            CopyArrayData<pConvAnimationNode->BindNode->TransformMatrix.Length>((double*)matRef, (float*)&matFlt);
        }

        auto vValue = matRef.GetQ();
        CopyArrayData(pConvOutRotation->raw, vValue.mData);
    }
    else{
        auto fTime = std::clamp(
            time,
            pConvAnimationNode->RotationKeys.Values[0].Time,
            pConvAnimationNode->RotationKeys.Values[pConvAnimationNode->RotationKeys.Length - 1].Time
        );
        ins_computeWorldByTime(pConvOutRotation->raw, fTime, &pConvAnimationNode->RotationKeys);
    }
}

__FBXM_MAKE_FUNC(void, FBXComputeAnimationLocalTranslation, void* pOutTranslation, const void* pAnimationNode, float time){
    const auto* pConvAnimationNode = reinterpret_cast<const FBXAnimationNode*>(pAnimationNode);

    Float3* pConvOutTranslation = reinterpret_cast<decltype(pConvOutTranslation)>(pOutTranslation);

    if(!pConvAnimationNode->TranslationKeys.Length){
        FbxAMatrix matRef;
        CopyArrayData<pConvAnimationNode->BindNode->TransformMatrix.Length>((double*)matRef, pConvAnimationNode->BindNode->TransformMatrix.Values);

        auto vValue = matRef.GetT();
        CopyArrayData(pConvOutTranslation->raw, vValue.mData);
    }
    else{
        auto fTime = std::clamp(
            time,
            pConvAnimationNode->TranslationKeys.Values[0].Time,
            pConvAnimationNode->TranslationKeys.Values[pConvAnimationNode->TranslationKeys.Length - 1].Time
        );
        ins_computeLocalByTime(pConvOutTranslation->raw, fTime, &pConvAnimationNode->TranslationKeys);
    }
}
__FBXM_MAKE_FUNC(void, FBXComputeAnimationWorldTranslation, void* pOutTranslation, const void* pAnimationNode, float time){
    const auto* pConvAnimationNode = reinterpret_cast<const FBXAnimationNode*>(pAnimationNode);

    Float3* pConvOutTranslation = reinterpret_cast<decltype(pConvOutTranslation)>(pOutTranslation);

    if(!pConvAnimationNode->TranslationKeys.Length){
        FbxAMatrix matRef;
        {
            auto xmm4_ret = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pConvAnimationNode->BindNode->TransformMatrix.Values);
            for(auto* pNode = pConvAnimationNode->BindNode->Parent; pNode; pNode = pNode->Parent){
                auto xmm4_tmp = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pNode->TransformMatrix.Values);
                xmm4_ret = DirectX::XMMatrixMultiply(xmm4_ret, xmm4_tmp);
            }
            DirectX::XMFLOAT4X4A matFlt;
            DirectX::XMStoreFloat4x4A(&matFlt, xmm4_ret);
            CopyArrayData<pConvAnimationNode->BindNode->TransformMatrix.Length>((double*)matRef, (float*)&matFlt);
        }

        auto vValue = matRef.GetT();
        CopyArrayData(pConvOutTranslation->raw, vValue.mData);
    }
    else{
        auto fTime = std::clamp(
            time,
            pConvAnimationNode->TranslationKeys.Values[0].Time,
            pConvAnimationNode->TranslationKeys.Values[pConvAnimationNode->TranslationKeys.Length - 1].Time
            );
        ins_computeWorldByTime(pConvOutTranslation->raw, fTime, &pConvAnimationNode->TranslationKeys);
    }
}
