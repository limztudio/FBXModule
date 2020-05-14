/**
* @file FBXUtilites.h
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include <intrin.h>

#include <eastl/string.h>

#include <fbxsdk.h>
#include <FBXNode.hpp>


struct alignas(16) XMMDouble{
    union{
        double d[2];
        __m128d v;
    };
};

static const XMMDouble XMMD_One = { { { 1., 1. } } };
static const XMMDouble XMMD_Epsilon = { { { DBL_EPSILON, DBL_EPSILON } } };
static const XMMDouble XMMD_EpsilonSq = { { { DBL_EPSILON * DBL_EPSILON, DBL_EPSILON * DBL_EPSILON } } };


struct Int3{
    union{
        struct{
            int x, y, z;
        };
        int raw[3];
    };
};


class CustomStream : public FbxStream{
public:
    CustomStream(FbxManager* kSDKManager, const char* fileName, const char* mode);
    virtual ~CustomStream();


public:
    virtual EState GetState(){ return m_file ? FbxStream::eOpen : eClosed; }

public:
    virtual bool Open(void* streamData);
    virtual bool Close();

public:
    virtual bool Flush(){ return true; }

public:
    virtual int Write(const void* data, int size);
    virtual int Read(void* data, int size)const;

public:
    virtual int GetReaderID()const{ return m_readerID; }
    virtual int GetWriterID()const{ return m_writerID; }

public:
    virtual void Seek(const FbxInt64& offset, const FbxFile::ESeekPos& seekPos);

    virtual long GetPosition()const;
    virtual void SetPosition(long position);

    virtual int GetError()const;
    virtual void ClearError();


private:
    eastl::string m_fileName;
    eastl::string m_fileMode;

    FILE* m_file;

    int m_readerID;
    int m_writerID;
};

extern void ConvertObjects(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene);

template<size_t LEN, typename T>
static inline size_t MakeHash(const T(&data)[LEN]){
    size_t c, result = 2166136261U; //FNV1 hash. Perhaps the best string hash
    for(const auto& i : data){
        c = static_cast<size_t>(i);
        result = (result * 16777619) ^ c;
    }
    return result;
}
template<template<typename> typename C, typename T>
static inline size_t MakeHash(const C<T>& data){
    size_t c, result = 2166136261U; //FNV1 hash. Perhaps the best string hash
    for(const auto& i : data){
        c = static_cast<size_t>(i);
        result = (result * 16777619) ^ c;
    }
    return result;
}
template<size_t LEN, typename T>
static inline size_t MakeHash(const T* data){
    size_t c, result = 2166136261U; //FNV1 hash. Perhaps the best string hash
    for(const auto* p = data; FBX_PTRDIFFU(p - data) < LEN; ++p){
        c = static_cast<size_t>(*p);
        result = (result * 16777619) ^ c;
    }
    return result;
}
template<typename T>
static inline size_t MakeHash(const T* data, size_t len){
    size_t c, result = 2166136261U; //FNV1 hash. Perhaps the best string hash
    for(const auto* p = data; FBX_PTRDIFFU(p - data) < len; ++p){
        c = static_cast<size_t>(*p);
        result = (result * 16777619) ^ c;
    }
    return result;
}

template<typename C>
static inline C* CopyString(const eastl::basic_string<C>& str){
    const auto last = str.length();
    auto* ret = reinterpret_cast<C*>(malloc((last + 1u) * sizeof(C)));

    if(ret){
        CopyMemory(ret, str.data(), last * sizeof(C));
        ret[last] = 0;
    }

    return ret;
}

template<size_t LEN_LHS, size_t LEN_RHS, typename LHS, typename RHS>
static inline void CopyArrayData(LHS(&lhs)[LEN_LHS], RHS(&&rhs)[LEN_RHS]){
    for(size_t i = 0; i < LEN_LHS; ++i)
        lhs[i] = eastl::move(static_cast<LHS>(rhs[i]));
}
template<size_t LEN_LHS, typename LHS, typename RHS>
static inline void CopyArrayData(LHS(&lhs)[LEN_LHS], const RHS* rhs){
    for(size_t i = 0; i < LEN_LHS; ++i)
        lhs[i] = static_cast<LHS>(rhs[i]);
}
template<size_t LEN_LHS, typename LHS, typename RHS>
static inline void CopyArrayData(LHS(&lhs)[LEN_LHS], const RHS& rhs){
    for(size_t i = 0; i < LEN_LHS; ++i)
        lhs[i] = static_cast<LHS>(rhs[i]);
}
template<size_t LEN_RHS, typename LHS, typename RHS>
static inline void CopyArrayData(LHS* lhs, RHS(&&rhs)[LEN_RHS]){
    for(size_t i = 0; i < LEN_RHS; ++i)
        lhs[i] = static_cast<LHS>(rhs[i]);
}
template<size_t LEN_RHS, template<typename> typename LHS, typename LHS_T, typename RHS>
static inline void CopyArrayData(LHS<LHS_T>& lhs, RHS(&&rhs)[LEN_RHS]){
    for(size_t i = 0; i < LEN_RHS; ++i)
        lhs[i] = static_cast<LHS_T>(rhs[i]);
}
template<size_t LEN, typename LHS, typename RHS>
static inline void CopyArrayData(LHS* lhs, const RHS* rhs){
    for(size_t i = 0; i < LEN; ++i)
        lhs[i] = static_cast<LHS>(rhs[i]);
}
template<size_t LEN, template<typename> typename LHS, typename LHS_T, typename RHS>
static inline void CopyArrayData(LHS<LHS_T>& lhs, const RHS* rhs){
    for(size_t i = 0; i < LEN; ++i)
        lhs[i] = static_cast<LHS_T>(rhs[i]);
}
template<typename LHS, typename RHS>
static inline void CopyArrayData(LHS* lhs, const RHS* rhs, size_t len){
    for(size_t i = 0; i < len; ++i)
        lhs[i] = static_cast<LHS>(rhs[i]);
}
template<template<typename> typename LHS, typename LHS_T, typename RHS>
static inline void CopyArrayData(LHS<LHS_T>& lhs, const RHS* rhs, size_t len){
    for(size_t i = 0; i < len; ++i)
        lhs[i] = static_cast<LHS_T>(rhs[i]);
}

static inline fbxsdk::FbxAMatrix GetLocalTransform(fbxsdk::FbxNode* kNode){
    return kNode->GetScene()->GetAnimationEvaluator()->GetNodeLocalTransform(kNode);
}
static inline fbxsdk::FbxAMatrix GetLocalTransform(fbxsdk::FbxNode* kNode, const fbxsdk::FbxTime& kTime){
    return kNode->GetScene()->GetAnimationEvaluator()->GetNodeLocalTransform(kNode, kTime);
}
static inline fbxsdk::FbxAMatrix GetGlobalTransform(fbxsdk::FbxNode* kNode){
    return kNode->GetScene()->GetAnimationEvaluator()->GetNodeGlobalTransform(kNode);
}
static inline fbxsdk::FbxAMatrix GetGlobalTransform(fbxsdk::FbxNode* kNode, const fbxsdk::FbxTime& kTime){
    return kNode->GetScene()->GetAnimationEvaluator()->GetNodeGlobalTransform(kNode, kTime);
}

static inline fbxsdk::FbxAMatrix GetGeometry(fbxsdk::FbxNode* kNode){
    fbxsdk::FbxVector4 kT, kR, kS;

    kT = kNode->GetGeometricTranslation(fbxsdk::FbxNode::eSourcePivot);
    kR = kNode->GetGeometricRotation(fbxsdk::FbxNode::eSourcePivot);
    kS = kNode->GetGeometricScaling(fbxsdk::FbxNode::eSourcePivot);

    return fbxsdk::FbxAMatrix(kT, kR, kS);
}

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
