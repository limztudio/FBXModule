/**
* @file FBXUtilites.h
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include <string>
#include <vector>
#include <unordered_map>

#include <fbxsdk.h>
#include <FBXNode.hpp>


template<typename KEY>
class CustomHasher{
public:
    inline size_t operator()(const KEY& k)const{ return ((size_t)k); }
};
template<typename KEY>
class PointerHasher{
public:
    inline size_t operator()(const KEY& k)const{
        auto v = reinterpret_cast<size_t>(k);
#ifdef _WIN64
        v >>= 3;
#else
        v >>= 2;
#endif
        return v;
    }
};


template<size_t LEN, typename T>
static inline size_t MakeHash(const T(&data)[LEN]){
    size_t c, result = 2166136261U; //FNV1 hash
    for(const auto& i : data){
        c = static_cast<size_t>(i);
        result = (result * 16777619) ^ c;
    }
    return result;
}
template<template<typename> typename C, typename T>
static inline size_t MakeHash(const C<T>& data){
    size_t c, result = 2166136261U; //FNV1 hash
    for(const auto& i : data){
        c = static_cast<size_t>(i);
        result = (result * 16777619) ^ c;
    }
    return result;
}
template<size_t LEN, typename T>
static inline size_t MakeHash(const T* data){
    size_t c, result = 2166136261U; //FNV1 hash
    for(const auto* p = data; FBX_PTRDIFFU(p - data) < LEN; ++p){
        c = static_cast<size_t>(*p);
        result = (result * 16777619) ^ c;
    }
    return result;
}
template<typename T>
static inline size_t MakeHash(const T* data, size_t len){
    size_t c, result = 2166136261U; //FNV1 hash
    for(const auto* p = data; FBX_PTRDIFFU(p - data) < len; ++p){
        c = static_cast<size_t>(*p);
        result = (result * 16777619) ^ c;
    }
    return result;
}


template<typename T>
class Container2{
public:
    Container2(){}
    Container2(const T& _x, const T& _y) : x(_x), y(_y){}
    Container2(T&& _x, T&& _y) : x(std::move(_x)), y(std::move(_y)){}
    Container2(const T* rhs) : x(rhs[0]), y(rhs[1]){}


public:
    inline operator size_t()const{ return MakeHash(raw); }


public:
    union{
        struct{
            T x, y;
        };
        T raw[2];
    };
};
template<typename T>
static inline bool operator==(const Container2<T>& lhs, const Container2<T>& rhs){
    if(lhs.x != rhs.x)
        return false;
    if(lhs.y != rhs.y)
        return false;
    return true;
}
template<typename T>
static inline bool operator!=(const Container2<T>& lhs, const Container2<T>& rhs){
    if((lhs.x == rhs.x) && (lhs.y == rhs.y))
        return false;
    return true;
}
template<typename T>
class Container3{
public:
    Container3(){}
    Container3(const T& _x, const T& _y, const T& _z) : x(_x), y(_y), z(_z){}
    Container3(T&& _x, T&& _y, T&& _z) : x(std::move(_x)), y(std::move(_y)), z(std::move(_z)){}
    Container3(const T* rhs) : x(rhs[0]), y(rhs[1]), z(rhs[2]){}


public:
    inline operator size_t()const{ return MakeHash(raw); }


public:
    union{
        struct{
            T x, y, z;
        };
        T raw[3];
    };
};
template<typename T>
static inline bool operator==(const Container3<T>& lhs, const Container3<T>& rhs){
    if(lhs.x != rhs.x)
        return false;
    if(lhs.y != rhs.y)
        return false;
    if(lhs.z != rhs.z)
        return false;
    return true;
}
template<typename T>
static inline bool operator!=(const Container3<T>& lhs, const Container3<T>& rhs){
    if((lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z))
        return false;
    return true;
}
template<typename T>
class Container4{
public:
    Container4(){}
    Container4(const T& _x, const T& _y, const T& _z, const T& _w) : x(_x), y(_y), z(_z), w(_w){}
    Container4(T&& _x, T&& _y, T&& _z, T&& _w) : x(std::move(_x)), y(std::move(_y)), z(std::move(_z)), w(std::move(_w)){}
    Container4(const T* rhs) : x(rhs[0]), y(rhs[1]), z(rhs[2]), w(rhs[3]){}


public:
    inline operator size_t()const{ return MakeHash(raw); }


public:
    union{
        struct{
            T x, y, z, w;
        };
        T raw[4];
    };
};
template<typename T>
static inline bool operator==(const Container4<T>& lhs, const Container4<T>& rhs){
    if(lhs.x != rhs.x)
        return false;
    if(lhs.y != rhs.y)
        return false;
    if(lhs.z != rhs.z)
        return false;
    if(lhs.w != rhs.w)
        return false;
    return true;
}
template<typename T>
static inline bool operator!=(const Container4<T>& lhs, const Container4<T>& rhs){
    if((lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z) && (lhs.w == rhs.w))
        return false;
    return true;
}

using Int2 = Container2<int>;
using Int3 = Container3<int>;
using Int4 = Container4<int>;

using Float2 = Container2<float>;
using Float3 = Container3<float>;
using Float4 = Container4<float>;


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
    std::string m_fileName;
    std::string m_fileMode;

    FILE* m_file;

    int m_readerID;
    int m_writerID;
};


namespace __hidden_FBXModule{
    template<typename T>
    class _OverlapReducer_Compare_Key{
    public:
        _OverlapReducer_Compare_Key(const _OverlapReducer_Compare_Key<T>& rhs)
            :
            hash(rhs.hash),
            data(rhs.data)
        {}
        _OverlapReducer_Compare_Key(const T& _data, size_t _hash)
            :
            hash(_hash),
            data(_data)
        {}


    public:
        inline operator size_t()const{ return hash; }


    public:
        const size_t hash;
        const T& data;
    };
};
template<typename T>
static inline bool operator==(const __hidden_FBXModule::_OverlapReducer_Compare_Key<T>& lhs, const __hidden_FBXModule::_OverlapReducer_Compare_Key<T>& rhs){
    return (lhs.data == rhs.data);
}
template<typename T>
class OverlapReducer{
public:
    OverlapReducer(){}
    template<typename FILL_FUNC>
    OverlapReducer(size_t len, FILL_FUNC func){
        m_oldData.resize(len);
        for(size_t i = 0; i < len; ++i)
            m_oldData[i] = func(i);
    }
    OverlapReducer(const std::vector<T>& data) : m_oldData(data){}
    OverlapReducer(std::vector<T>&& data) : m_oldData(std::move(data)){}


public:
    template<typename FILL_FUNC>
    inline void init(size_t len, FILL_FUNC func){
        m_oldData.resize(len);
        for(size_t i = 0; i < len; ++i)
            m_oldData[i] = func(i);
    }
    inline void init(const std::vector<T>& data){ m_oldData = data; }
    inline void init(std::vector<T>&& data){ m_oldData = std::(data); }


public:
    template<typename HASH_FUNC>
    inline void build(HASH_FUNC hashFunc){
        const auto oldSize = m_oldData.size();

        m_convData.clear();
        m_convData.reserve(oldSize);

        m_oldToConvIndexer.clear();
        m_oldToConvIndexer.reserve(oldSize);

        m_comparer.clear();
        m_comparer.rehash(oldSize << 2);

        for(size_t idxOld = 0; idxOld < oldSize; ++idxOld){
            const auto& iOld = m_oldData[idxOld];

            T iNew(iOld);
            const size_t iNewHash = hashFunc(iNew);

            auto f = m_comparer.find(__hidden_FBXModule::_OverlapReducer_Compare_Key<T>(iNew, iNewHash));
            if(f == m_comparer.cend()){
                const size_t idxNew = m_convData.size();

                m_convData.emplace_back(std::move(iNew));
                m_oldToConvIndexer.emplace_back(idxNew);

                m_comparer.emplace(__hidden_FBXModule::_OverlapReducer_Compare_Key<T>(m_convData[idxNew], iNewHash), idxNew);
            }
            else
                m_oldToConvIndexer.emplace_back(f->second);
        }
    }

public:
    inline const std::vector<T>& getConvertedTable()const{ return m_convData; }
    inline std::vector<T>& getConvertedTable(){ return m_convData; }

    inline const std::vector<size_t>& getOldToConvertIndexer()const{ return m_oldToConvIndexer; }
    inline std::vector<size_t>& getOldToConvertIndexer(){ return m_oldToConvIndexer; }


private:
    std::vector<T> m_oldData;
    std::vector<T> m_convData;

    std::vector<size_t> m_oldToConvIndexer;
    std::unordered_map<__hidden_FBXModule::_OverlapReducer_Compare_Key<T>, size_t, CustomHasher<__hidden_FBXModule::_OverlapReducer_Compare_Key<T>>> m_comparer;
};


extern void ConvertObjects(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene);

template<size_t LEN_LHS, size_t LEN_RHS, typename LHS, typename RHS, typename INDEX_TYPE = size_t>
static inline void CopyArrayData(LHS(&lhs)[LEN_LHS], RHS(&&rhs)[LEN_RHS]){
    for(INDEX_TYPE i = 0; i < INDEX_TYPE(LEN_LHS); ++i)
        lhs[i] = std::move(static_cast<LHS>(rhs[i]));
}
template<size_t LEN_LHS, typename LHS, typename RHS, typename INDEX_TYPE = size_t>
static inline void CopyArrayData(LHS(&lhs)[LEN_LHS], const RHS* rhs){
    for(INDEX_TYPE i = 0; i < INDEX_TYPE(LEN_LHS); ++i)
        lhs[i] = static_cast<LHS>(rhs[i]);
}
template<size_t LEN_LHS, typename LHS, typename RHS, typename INDEX_TYPE = size_t>
static inline void CopyArrayData(LHS(&lhs)[LEN_LHS], const RHS& rhs){
    for(INDEX_TYPE i = 0; i < INDEX_TYPE(LEN_LHS); ++i)
        lhs[i] = static_cast<LHS>(rhs[i]);
}
template<size_t LEN_RHS, typename LHS, typename RHS, typename INDEX_TYPE = size_t>
static inline void CopyArrayData(LHS* lhs, RHS(&&rhs)[LEN_RHS]){
    for(INDEX_TYPE i = 0; i < INDEX_TYPE(LEN_RHS); ++i)
        lhs[i] = static_cast<LHS>(rhs[i]);
}
template<size_t LEN_RHS, template<typename> typename LHS, typename LHS_T, typename RHS, typename INDEX_TYPE = size_t>
static inline void CopyArrayData(LHS<LHS_T>& lhs, RHS(&&rhs)[LEN_RHS]){
    for(INDEX_TYPE i = 0; i < INDEX_TYPE(LEN_RHS); ++i)
        lhs[i] = static_cast<LHS_T>(rhs[i]);
}
template<size_t LEN, typename LHS, typename RHS, typename INDEX_TYPE = size_t>
static inline void CopyArrayData(LHS* lhs, const RHS* rhs){
    for(INDEX_TYPE i = 0; i < INDEX_TYPE(LEN); ++i)
        lhs[i] = static_cast<LHS>(rhs[i]);
}
template<size_t LEN, template<typename> typename LHS, typename LHS_T, typename RHS, typename INDEX_TYPE = size_t>
static inline void CopyArrayData(LHS<LHS_T>& lhs, const RHS* rhs){
    for(INDEX_TYPE i = 0; i < INDEX_TYPE(LEN); ++i)
        lhs[i] = static_cast<LHS_T>(rhs[i]);
}
template<typename LHS, typename RHS, typename INDEX_TYPE = size_t>
static inline void CopyArrayData(LHS* lhs, const RHS* rhs, INDEX_TYPE len){
    for(INDEX_TYPE i = 0; i < len; ++i)
        lhs[i] = static_cast<LHS>(rhs[i]);
}
template<template<typename> typename LHS, typename LHS_T, typename RHS, typename INDEX_TYPE = size_t>
static inline void CopyArrayData(LHS<LHS_T>& lhs, const RHS* rhs, INDEX_TYPE len){
    for(INDEX_TYPE i = 0; i < len; ++i)
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
