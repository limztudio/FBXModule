/**
 * @file FBXMath.h
 * @date 2020/05/21
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#pragma once


#define _XM_SSE4_INTRINSICS_

#ifdef _SIMD_AVX
#define _XM_AVX2_INTRINSICS_
#endif

#ifdef _SIMD_FMA
#define _XM_FMA3_INTRINSICS_
#endif

#include "DirectXMath/Inc/DirectXMath.h"

#include "DirectXMath/Extensions/DirectXMathSSE4.h"

#ifdef _SIMD_AVX
#include "DirectXMath/Extensions/DirectXMathAVX2.h"
#endif

#ifdef _SIMD_FMA
#include "DirectXMath/Extensions/DirectXMathFMA3.h"
#endif

#include "FBXUtilites.h"


struct alignas(16) XMMDouble{
    union{
        double d[2];
        __m128d v;
    };
};


static const XMMDouble XMMD_One = { { { 1., 1. } } };
static const XMMDouble XMMD_Epsilon = { { { DBL_EPSILON, DBL_EPSILON } } };
static const XMMDouble XMMD_EpsilonSq = { { { DBL_EPSILON * DBL_EPSILON, DBL_EPSILON * DBL_EPSILON } } };


extern bool SIMDCompetible();


static inline fbxsdk::FbxDouble4x4 Scale44(const fbxsdk::FbxDouble4x4& kMatrix, const fbxsdk::FbxDouble& kFactor){
    alignas(16) fbxsdk::FbxDouble4x4 kOut44;

    auto xmm_factor = _mm_set1_pd(kFactor);

    for(int i = 0; i < 4; ++i){
        const auto& kRow = kMatrix[i];
        auto& kOutRow = kOut44[i];

        auto xmm_xy = _mm_loadu_pd(kRow.Buffer());
        auto xmm_zw = _mm_loadu_pd(kRow.Buffer() + 2);

        xmm_xy = _mm_mul_pd(xmm_xy, xmm_factor);
        xmm_zw = _mm_mul_pd(xmm_zw, xmm_factor);

        _mm_store_pd(kOutRow.Buffer(), xmm_xy);
        _mm_store_pd(kOutRow.Buffer() + 2, xmm_zw);
    }

    return kOut44;
}

static inline fbxsdk::FbxDouble4x4 Add44(const fbxsdk::FbxDouble4x4& kLhs, const fbxsdk::FbxDouble4x4& kRhs){
    alignas(16) fbxsdk::FbxDouble4x4 kOut44;

    for(int i = 0; i < 4; ++i){
        const auto& kRowLhs = kLhs[i];
        const auto& kRowRhs = kRhs[i];
        auto& kOutRow = kOut44[i];

        auto xmm_xy_lhs = _mm_loadu_pd(kRowLhs.Buffer());
        auto xmm_zw_lhs = _mm_loadu_pd(kRowLhs.Buffer() + 2);

        auto xmm_xy_rhs = _mm_loadu_pd(kRowRhs.Buffer());
        auto xmm_zw_rhs = _mm_loadu_pd(kRowRhs.Buffer() + 2);

        auto xmm_xy = _mm_add_pd(xmm_xy_lhs, xmm_xy_rhs);
        auto xmm_zw = _mm_add_pd(xmm_zw_lhs, xmm_zw_rhs);

        _mm_store_pd(kOutRow.Buffer(), xmm_xy);
        _mm_store_pd(kOutRow.Buffer() + 2, xmm_zw);
    }

    return kOut44;
}

static inline fbxsdk::FbxDouble3 Transform44(const fbxsdk::FbxDouble4x4& kMatrix, const fbxsdk::FbxDouble3& kVector3){
    fbxsdk::FbxDouble4 kOut4;

    for(int i = 0; i < 4; ++i){
        kOut4[i] = kVector3[0] * kMatrix[0][i];
        kOut4[i] += kVector3[1] * kMatrix[1][i];
        kOut4[i] += kVector3[2] * kMatrix[2][i];
        kOut4[i] += kMatrix[3][i];
    }

    fbxsdk::FbxDouble3 kOut3;
    {
        kOut3[0] = kOut4[0];
        kOut3[1] = kOut4[1];
        kOut3[2] = kOut4[2];
    }

    {
        auto& w = kOut4[3];

        kOut3[0] /= w;
        kOut3[1] /= w;
        kOut3[2] /= w;
    }

    return kOut3;
}
static inline fbxsdk::FbxDouble3 Transform33(const fbxsdk::FbxDouble4x4& kMatrix, const fbxsdk::FbxDouble3& kVector3){
    fbxsdk::FbxDouble3 kOut3;

    for(int i = 0; i < 3; ++i){
        kOut3[i] = kVector3[0] * kMatrix[0][i];
        kOut3[i] += kVector3[1] * kMatrix[1][i];
        kOut3[i] += kVector3[2] * kMatrix[2][i];
    }

    return kOut3;
}

static inline fbxsdk::FbxDouble3 Normalize3(const fbxsdk::FbxDouble3& kVector3){
    auto xmm_xy = _mm_loadu_pd(kVector3.Buffer());
    auto xmm_zw = _mm_loadu_pd(kVector3.Buffer() + 2);

#ifdef _SIMD_FMA
    auto xmm_lenSq = _mm_dp_pd(xmm_xy, xmm_xy, 0xf1);
    xmm_lenSq = _mm_fmadd_pd(xmm_zw, xmm_zw, xmm_lenSq);
#else
    auto xmm_zwSq = _mm_mul_pd(xmm_zw, xmm_zw);

    auto xmm_lenSq = _mm_dp_pd(xmm_xy, xmm_xy, 0xf1);
    xmm_lenSq = _mm_add_pd(xmm_lenSq, xmm_zwSq);
#endif

    if((_mm_movemask_pd(_mm_cmpge_pd(xmm_lenSq, XMMD_EpsilonSq.v)) & 1) == 1){
#ifdef _SIMD_AVX
        xmm_lenSq = _mm_permute_pd(xmm_lenSq, _MM_SHUFFLE2(0, 0));
#else
        xmm_lenSq = _mm_shuffle_pd(xmm_lenSq, xmm_lenSq, _MM_SHUFFLE2(0, 0));
#endif
        auto xmm_len = _mm_sqrt_pd(xmm_lenSq);

        xmm_xy = _mm_div_pd(xmm_xy, xmm_len);
        xmm_zw = _mm_div_pd(xmm_zw, xmm_len);

        alignas(16) fbxsdk::FbxDouble3 kRet;
        _mm_store_pd(kRet.Buffer(), xmm_xy);
        kRet[2] = _mm_cvtsd_f64(xmm_zw);

        return kRet;
    }

    return fbxsdk::FbxDouble3(0., 0., 0.);
}

static inline Float3 Normalize3(const Float3& flt3){
    auto xmm_v = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)flt3.raw);

    xmm_v = DirectX::XMVector3Normalize(xmm_v);

    Float3 ret;
    DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)ret.raw, xmm_v);

    return ret;
}
