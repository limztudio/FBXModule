/**
* @file FBXModule.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <unordered_map>

#include "FBXMath.h"
#include "FBXShared.h"


static inline DirectX::XMMATRIX __vectorcall ins_getLocalMatrix(const FBXNode* pNode){
    return DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pNode->TransformMatrix.Values);
}
static inline DirectX::XMMATRIX __vectorcall ins_getWorldMatrix(const FBXNode* pTargetNode){
    auto xmm4_ret = ins_getLocalMatrix(pTargetNode);

    for(const auto* pNode = pTargetNode->Parent; pNode; pNode = pNode->Parent)
        xmm4_ret = DirectX::XMMatrixMultiply(xmm4_ret, ins_getWorldMatrix(pNode));

    return xmm4_ret;
}


__FBXM_MAKE_FUNC(const void*, FBXGetRoot, void){
    return shr_root;
}
__FBXM_MAKE_FUNC(void, FBXCopyRoot, void* pDest, const void* pSrc){
    auto* dest = static_cast<FBXRoot*>(pDest);
    const auto* src = static_cast<const FBXRoot*>(pSrc);

    SHRCopyRoot(dest, src);
}

__FBXM_MAKE_FUNC(void, FBXGetWorldMatrix, void* pOutMatrix, const void* pNode){
    const auto* pConvNode = reinterpret_cast<const FBXNode*>(pNode);

    FbxAMatrix kMatRes;
    CopyArrayData<16>((double*)kMatRes, pConvNode->TransformMatrix.Values);
    for(pConvNode = pConvNode->Parent; pConvNode; pConvNode = pConvNode->Parent){
        FbxAMatrix kMatTmp;
        CopyArrayData<16>((double*)kMatTmp, pConvNode->TransformMatrix.Values);
        kMatRes = kMatTmp * kMatRes;
    }
    CopyArrayData<16>((float*)pOutMatrix, (double*)kMatRes);

    //auto xmm4_ret = ins_getWorldMatrix(pConvNode);
    //DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)pOutMatrix, xmm4_ret);
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
