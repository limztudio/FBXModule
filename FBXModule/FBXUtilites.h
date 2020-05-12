/**
* @file FBXUtilites.h
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include <eastl/string.h>

#include <fbxsdk.h>
#include <FBXNode.hpp>


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
template<size_t LEN_RHS, typename LHS, typename RHS>
static inline void CopyArrayData(LHS* lhs, RHS(&&rhs)[LEN_RHS]){
    for(size_t i = 0; i < LEN_RHS; ++i)
        lhs[i] = static_cast<LHS>(rhs[i]);
}
template<size_t LEN, typename LHS, typename RHS>
static inline void CopyArrayData(LHS* lhs, const RHS* rhs){
    for(size_t i = 0; i < LEN; ++i)
        lhs[i] = static_cast<LHS>(rhs[i]);
}
template<typename LHS, typename RHS>
static inline void CopyArrayData(LHS* lhs, const RHS* rhs, size_t len){
    for(size_t i = 0; i < len; ++i)
        lhs[i] = static_cast<LHS>(rhs[i]);
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
    fbxsdk::FbxDouble4x4 kOut44;

    for(int i = 0; i < 4; ++i){
        const auto& kRow = kMatrix[i];
        auto& kOutRow = kOut44[i];
        for(int j = 0; j < 4; ++j)
            kOutRow[j] = kRow[j] * kFactor;
    }

    return kOut44;
}

static inline fbxsdk::FbxDouble4x4 Add44(const fbxsdk::FbxDouble4x4& kLhs, const fbxsdk::FbxDouble4x4& kRhs){
    fbxsdk::FbxDouble4x4 kOut44;

    for(int i = 0; i < 4; ++i){
        const auto& kRowLhs = kLhs[i];
        const auto& kRowRhs = kRhs[i];
        auto& kOutRow = kOut44[i];
        for(int j = 0; j < 4; ++j)
            kOutRow[j] = kRowLhs[j] + kRowRhs[j];
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

static inline fbxsdk::FbxDouble3 Normalize3(fbxsdk::FbxDouble3 kVector3){
    auto x = kVector3[0];
    auto y = kVector3[1];
    auto z = kVector3[2];

    x *= x;
    y *= y;
    z *= z;

    auto lenSq = x + y + z;

    if(lenSq < (DBL_EPSILON * DBL_EPSILON)){
        kVector3[0] = 0;
        kVector3[1] = 0;
        kVector3[2] = 0;
    }
    else{
        auto len = ::sqrt(lenSq);

        kVector3[0] = x / len;
        kVector3[1] = y / len;
        kVector3[2] = z / len;
    }

    return kVector3;
}
