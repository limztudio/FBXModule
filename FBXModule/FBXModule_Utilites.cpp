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
static inline void ins_interpolateValue(FBXStaticArray<T, N>* pOut, const FBXStaticArray<T, N>* pV0, const FBXStaticArray<T, N>* pV1, float t){}
template<>
inline void ins_interpolateValue(FBXStaticArray<float, 3>* pOut, const FBXStaticArray<float, 3>* pV0, const FBXStaticArray<float, 3>* pV1, float t){
    auto xmm_v0 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)pV0->Values);
    auto xmm_v1 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)pV1->Values);

    auto xmm_v = DirectX::XMVectorLerp(xmm_v0, xmm_v1, t);
    DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)pOut->Values, xmm_v);
}
template<>
inline void ins_interpolateValue(FBXStaticArray<float, 4>* pOut, const FBXStaticArray<float, 4>* pV0, const FBXStaticArray<float, 4>* pV1, float t){
    auto xmm_v0 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)pV0->Values);
    auto xmm_v1 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)pV1->Values);

    auto xmm_v = DirectX::XMQuaternionSlerp(xmm_v0, xmm_v1, t);
    xmm_v = DirectX::XMQuaternionNormalize(xmm_v);
    DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)pOut->Values, xmm_v);
}

template<typename T, unsigned long N>
static inline void ins_computeValueByTyime(FBXStaticArray<T, N>* pOut, float time, const FBXDynamicArray<FBXAnimationKeyFrame<FBXStaticArray<T, N>>>* pTable){
    const FBXAnimationKeyFrame<FBXStaticArray<T, N>> *pData = pTable->Values, *pOldData = pData;
    for(; FBX_PTRDIFFU(pData - pTable->Values) < pTable->Length; pOldData = pData++){
        if(pData->Time > time){
            switch(pOldData->InterpolationType){
            case FBXAnimationInterpolationType::FBXAnimationInterpolationType_Stepped:
            {
                (*pOut) = pOldData->Value;
                return;
            }
            case FBXAnimationInterpolationType::FBXAnimationInterpolationType_Linear:
            {
                auto fTime = (time - pOldData->Time) / (pData->Time - pOldData->Time);
                ins_interpolateValue<T, N>(pOut, &pOldData->Value, &pData->Value, fTime);
                return;
            }
            }
        }
    }

    (*pOut) = pOldData->Value;
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


__FBXM_MAKE_FUNC(void, FBXComputeAnimationTransform, void* pOutScale, void* pOutRotation, void* pOutTranslation, const void* pAnimationNode, float time){
    const auto* pConvAnimationNode = reinterpret_cast<const FBXAnimationNode*>(pAnimationNode);

    FBXStaticArray<float, 3>* pConvOutScale = reinterpret_cast<decltype(pConvOutScale)>(pOutScale);
    FBXStaticArray<float, 4>* pConvOutRotation = reinterpret_cast<decltype(pConvOutRotation)>(pOutRotation);
    FBXStaticArray<float, 3>* pConvOutTranslation = reinterpret_cast<decltype(pConvOutTranslation)>(pOutTranslation);

    FbxAMatrix matRef;
    CopyArrayData<pConvAnimationNode->BindNode->TransformMatrix.Length>((double*)matRef, pConvAnimationNode->BindNode->TransformMatrix.Values);

    if(!pConvAnimationNode->ScalingKeys.Length){
        auto vValue = matRef.GetS();
        CopyArrayData(pConvOutScale->Values, vValue.mData);
    }
    else{
        auto fTime = std::min(time, pConvAnimationNode->ScalingKeys.Values[pConvAnimationNode->ScalingKeys.Length - 1].Time);
        ins_computeValueByTyime(pConvOutScale, fTime, &pConvAnimationNode->ScalingKeys);
    }

    if(!pConvAnimationNode->RotationKeys.Length){
        auto vValue = matRef.GetQ();
        CopyArrayData(pConvOutRotation->Values, vValue.mData);
    }
    else{
        auto fTime = std::min(time, pConvAnimationNode->RotationKeys.Values[pConvAnimationNode->RotationKeys.Length - 1].Time);
        ins_computeValueByTyime(pConvOutRotation, fTime, &pConvAnimationNode->RotationKeys);
    }

    if(!pConvAnimationNode->TranslationKeys.Length){
        auto vValue = matRef.GetT();
        CopyArrayData(pConvOutTranslation->Values, vValue.mData);
    }
    else{
        auto fTime = std::min(time, pConvAnimationNode->TranslationKeys.Values[pConvAnimationNode->TranslationKeys.Length - 1].Time);
        ins_computeValueByTyime(pConvOutTranslation, fTime, &pConvAnimationNode->TranslationKeys);
    }
}
