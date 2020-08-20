/**
* @file FBXShared_Optimizer.cpp
* @date 2020/05/13
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <FBXAssign.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


using namespace fbxsdk;


static fbx_unordered_set<const FbxCluster*, PointerHasher<const FbxCluster*>> ins_nodeUsageChecker;


class _VertexInfo{
public:
    static inline size_t makeHash(const _VertexInfo& data){
        size_t c, result = 2166136261U; //FNV1 hash

#define __CAL_RESULT result = (result * 16777619) ^ c;

        {
            c = MakeHash(data.position.mData);
            __CAL_RESULT;
        }

        for(const auto& i : data.skinData){
            c = MakeHash<1>(&i.weight);
            __CAL_RESULT;

            c = reinterpret_cast<size_t>(i.cluster);
            __CAL_RESULT;
        }

        for(const auto& i : data.layeredColor){
            c = MakeHash(i.mData);
            __CAL_RESULT;
        }

        for(const auto& i : data.layeredNormal){
            c = MakeHash(i.mData);
            __CAL_RESULT;
        }

        for(const auto& i : data.layeredBinormal){
            c = MakeHash(i.mData);
            __CAL_RESULT;
        }

        for(const auto& i : data.layeredTangent){
            c = MakeHash(i.mData);
            __CAL_RESULT;
        }

        for(const auto& i : data.layeredUV){
            c = MakeHash(i.first.c_str(), i.first.length());
            __CAL_RESULT;

            c = MakeHash(i.second.mData);
            __CAL_RESULT;
        }

#undef __CAL_RESULT

        return result;
    }


public:
    fbxsdk::FbxDouble3 position;

    fbx_vector<SkinInfo> skinData;

    Vector4Container layeredColor;

    Unit3Container layeredNormal;
    Unit3Container layeredBinormal;
    Unit3Container layeredTangent;

    fbx_vector<std::pair<fbx_basic_string<char>, fbxsdk::FbxDouble2>> layeredUV;
};
static inline bool operator==(const _VertexInfo& lhs, const _VertexInfo& rhs){
    {
        const auto& lhsVal = lhs.position;
        const auto& rhsVal = rhs.position;

        if(lhsVal.mData[0] != rhsVal.mData[0])
            return false;
        if(lhsVal.mData[1] != rhsVal.mData[1])
            return false;
        if(lhsVal.mData[2] != rhsVal.mData[2])
            return false;
    }

    if(lhs.skinData.size() != rhs.skinData.size())
        return false;
    for(auto edx = (unsigned int)lhs.skinData.size(), idx = 0u; idx < edx; ++idx){
        const auto& lhsSkin = lhs.skinData[idx];
        const auto& rhsSkin = rhs.skinData[idx];
        if(lhsSkin.weight != rhsSkin.weight)
            return false;
        if(lhsSkin.cluster != rhsSkin.cluster)
            return false;
    }

    if(lhs.layeredColor.size() != rhs.layeredColor.size())
        return false;
    for(auto edx = (unsigned int)lhs.layeredColor.size(), idx = 0u; idx < edx; ++idx){
        const auto& lhsVal = lhs.layeredColor[idx];
        const auto& rhsVal = rhs.layeredColor[idx];

        if(lhsVal.mData[0] != rhsVal.mData[0])
            return false;
        if(lhsVal.mData[1] != rhsVal.mData[1])
            return false;
        if(lhsVal.mData[2] != rhsVal.mData[2])
            return false;
        if(lhsVal.mData[3] != rhsVal.mData[3])
            return false;
    }

    if(lhs.layeredNormal.size() != rhs.layeredNormal.size())
        return false;
    for(auto edx = (unsigned int)lhs.layeredNormal.size(), idx = 0u; idx < edx; ++idx){
        const auto& lhsVal = lhs.layeredNormal[idx];
        const auto& rhsVal = rhs.layeredNormal[idx];

        if(lhsVal.mData[0] != rhsVal.mData[0])
            return false;
        if(lhsVal.mData[1] != rhsVal.mData[1])
            return false;
        if(lhsVal.mData[2] != rhsVal.mData[2])
            return false;
    }

    if(lhs.layeredBinormal.size() != rhs.layeredBinormal.size())
        return false;
    for(auto edx = (unsigned int)lhs.layeredBinormal.size(), idx = 0u; idx < edx; ++idx){
        const auto& lhsVal = lhs.layeredBinormal[idx];
        const auto& rhsVal = rhs.layeredBinormal[idx];

        if(lhsVal.mData[0] != rhsVal.mData[0])
            return false;
        if(lhsVal.mData[1] != rhsVal.mData[1])
            return false;
        if(lhsVal.mData[2] != rhsVal.mData[2])
            return false;
    }

    if(lhs.layeredTangent.size() != rhs.layeredTangent.size())
        return false;
    for(auto edx = (unsigned int)lhs.layeredTangent.size(), idx = 0u; idx < edx; ++idx){
        const auto& lhsVal = lhs.layeredTangent[idx];
        const auto& rhsVal = rhs.layeredTangent[idx];

        if(lhsVal.mData[0] != rhsVal.mData[0])
            return false;
        if(lhsVal.mData[1] != rhsVal.mData[1])
            return false;
        if(lhsVal.mData[2] != rhsVal.mData[2])
            return false;
    }

    if(lhs.layeredUV.size() != rhs.layeredUV.size())
        return false;
    for(auto edx = (unsigned int)lhs.layeredUV.size(), idx = 0u; idx < edx; ++idx){
        const auto& lhsVal = lhs.layeredUV[idx];
        const auto& rhsVal = rhs.layeredUV[idx];

        if(lhsVal.second.mData[0] != rhsVal.second.mData[0])
            return false;
        if(lhsVal.second.mData[1] != rhsVal.second.mData[1])
            return false;

        if(lhsVal.first != rhsVal.first)
            return false;
    }

    return true;
}

class _PolygonInfo{
public:
    inline bool useSameMaterial(const _PolygonInfo& rhs)const{
        if(layeredMaterial.size() != rhs.layeredMaterial.size())
            return false;
        for(auto edx = (unsigned int)layeredMaterial.size(), idx = 0u; idx < edx; ++idx){
            if(layeredMaterial[idx] != rhs.layeredMaterial[idx])
                return false;
        }

        return true;
    }


public:
    Uint3 indices;
    UintContainer layeredMaterial;
};

class _VertexInfoKey{
public:
    _VertexInfoKey(const _VertexInfo& _data, size_t _hash)
        :
        hash(_hash),
        data(_data)
    {}


public:
    inline operator size_t()const{ return hash; }


public:
    const size_t hash;
    const _VertexInfo& data;
};
static inline bool operator==(const _VertexInfoKey& lhs, const _VertexInfoKey& rhs){
    return (lhs.data == rhs.data);
}


static fbx_vector<_VertexInfo> ins_aosVertices;
static fbx_vector<_PolygonInfo> ins_aosPolygons;

static fbx_unordered_map<_VertexInfoKey, unsigned int, CustomHasher<_VertexInfoKey>> ins_aosVertexFinder;
static fbx_vector<unsigned int> ins_flatVertexBinder;


static inline void ins_fillAOSContainers(const NodeData* pNodeData){
    const auto layerCount = pNodeData->bufLayers.size();
    { // reserve
        const auto vertexCount = pNodeData->bufPositions.size();
        const auto polyCount = pNodeData->bufIndices.size();

        ins_flatVertexBinder.resize(vertexCount);

        ins_aosVertexFinder.clear();
        ins_aosVertexFinder.rehash(vertexCount << 1);

        ins_aosVertices.clear();
        ins_aosVertices.reserve(vertexCount);

        ins_aosPolygons.resize(polyCount);
        for(auto& iPoly : ins_aosPolygons){
            iPoly.layeredMaterial.resize(layerCount);
        }
    }

    for(auto edxPoly = (unsigned int)pNodeData->bufIndices.size(), idxPoly = 0u; idxPoly < edxPoly; ++idxPoly){
        const auto& iPoly = pNodeData->bufIndices[idxPoly];

        auto& iPolyInfo = ins_aosPolygons[idxPoly];

        for(auto edxLayer = (unsigned int)pNodeData->bufLayers.size(), idxLayer = 0u; idxLayer < edxLayer; ++idxLayer){
            const auto& iLayer = pNodeData->bufLayers[idxLayer];

            iPolyInfo.layeredMaterial[idxLayer] = (!iLayer.materials.empty()) ? iLayer.materials[idxPoly] : (-1);
        }

        for(size_t idxLocalVert = 0u; idxLocalVert < 3u; ++idxLocalVert){
            const auto idxVert = iPoly.raw[idxLocalVert];

            unsigned int idxVertInfo;
            {
                _VertexInfo iVertInfo;
                {
                    iVertInfo.position = pNodeData->bufPositions[idxVert];

                    if(!pNodeData->bufSkinData.empty())
                        iVertInfo.skinData = pNodeData->bufSkinData[idxVert];

                    size_t colorCount = 0u;
                    size_t normalCount = 0u;
                    size_t binormalCount = 0u;
                    size_t tangentCount = 0u;
                    size_t uvCount = 0u;

                    for(size_t idxLayer = 0u; idxLayer < layerCount; ++idxLayer){
                        const auto& iLayer = pNodeData->bufLayers[idxLayer];

                        if(!iLayer.colors.empty())
                            ++colorCount;
                        if(!iLayer.normals.empty())
                            ++normalCount;
                        if(!iLayer.binormals.empty())
                            ++binormalCount;
                        if(!iLayer.tangents.empty())
                            ++tangentCount;
                        if(!iLayer.texcoords.table.empty())
                            ++uvCount;
                    }

                    iVertInfo.layeredColor.resize(colorCount);
                    iVertInfo.layeredNormal.resize(normalCount);
                    iVertInfo.layeredBinormal.resize(binormalCount);
                    iVertInfo.layeredTangent.resize(tangentCount);
                    iVertInfo.layeredUV.resize(uvCount);

                    for(auto edxLayer = (unsigned int)iVertInfo.layeredColor.size(), idxLayer = 0u; idxLayer < edxLayer; ++idxLayer){
                        const auto& iLayer = pNodeData->bufLayers[idxLayer];

                        iVertInfo.layeredColor[idxLayer] = iLayer.colors[idxVert];
                    }
                    for(auto edxLayer = (unsigned int)iVertInfo.layeredNormal.size(), idxLayer = 0u; idxLayer < edxLayer; ++idxLayer){
                        const auto& iLayer = pNodeData->bufLayers[idxLayer];

                        iVertInfo.layeredNormal[idxLayer] = iLayer.normals[idxVert];
                    }
                    for(auto edxLayer = (unsigned int)iVertInfo.layeredBinormal.size(), idxLayer = 0u; idxLayer < edxLayer; ++idxLayer){
                        const auto& iLayer = pNodeData->bufLayers[idxLayer];

                        iVertInfo.layeredBinormal[idxLayer] = iLayer.binormals[idxVert];
                    }
                    for(auto edxLayer = (unsigned int)iVertInfo.layeredTangent.size(), idxLayer = 0u; idxLayer < edxLayer; ++idxLayer){
                        const auto& iLayer = pNodeData->bufLayers[idxLayer];

                        iVertInfo.layeredTangent[idxLayer] = iLayer.tangents[idxVert];
                    }
                    for(auto edxLayer = (unsigned int)iVertInfo.layeredUV.size(), idxLayer = 0u; idxLayer < edxLayer; ++idxLayer){
                        const auto& iLayer = pNodeData->bufLayers[idxLayer];

                        auto& v = iVertInfo.layeredUV[idxLayer];
                        v.first = iLayer.texcoords.name;
                        v.second = iLayer.texcoords.table[idxVert];
                    }
                }

                const auto iVertInfoHash = _VertexInfo::makeHash(iVertInfo);
                auto fVertexInfo = ins_aosVertexFinder.find(_VertexInfoKey(iVertInfo, iVertInfoHash));
                if(fVertexInfo == ins_aosVertexFinder.end()){
                    idxVertInfo = decltype(idxVertInfo)(ins_aosVertices.size());
                    ins_aosVertices.emplace_back(std::move(iVertInfo));

                    ins_aosVertexFinder.emplace(_VertexInfoKey(ins_aosVertices[idxVertInfo], iVertInfoHash), idxVertInfo);
                }
                else{
                    idxVertInfo = fVertexInfo->second;
                }
            }

            iPolyInfo.indices.raw[idxLocalVert] = idxVertInfo;

            ins_flatVertexBinder[idxVert] = idxVertInfo;
        }
    }
}

static inline void ins_genOptimizeMesh(NodeData* pNodeData){
    const bool isSkinned = (!pNodeData->bufSkinData.empty());

    {
        pNodeData->bufIndices.clear();
        for(const auto& iPolyInfo : ins_aosPolygons)
            pNodeData->bufIndices.emplace_back(iPolyInfo.indices);
    }
    {
        pNodeData->bufPositions.clear();
        for(const auto& iVertInfo : ins_aosVertices)
            pNodeData->bufPositions.emplace_back(iVertInfo.position);
    }
    if(isSkinned){
        pNodeData->bufSkinData.clear();
        for(const auto& iVertInfo : ins_aosVertices)
            pNodeData->bufSkinData.emplace_back(std::move(iVertInfo.skinData));
    }

    for(auto edxLayer = (unsigned int)pNodeData->bufLayers.size(), idxLayer = 0u; idxLayer < edxLayer; ++idxLayer){
        auto& iLayer = pNodeData->bufLayers[idxLayer];

        if(!iLayer.materials.empty()){
            iLayer.materials.clear();
            for(const auto& iPolyInfo : ins_aosPolygons){
                const auto& curMat = iPolyInfo.layeredMaterial[idxLayer];
                if(curMat >= 0u)
                    iLayer.materials.emplace_back((unsigned int)curMat);
            }
        }

        if(!iLayer.colors.empty()){
            iLayer.colors.clear();
            for(const auto& iVertInfo : ins_aosVertices)
                iLayer.colors.emplace_back(iVertInfo.layeredColor[idxLayer]);
        }

        if(!iLayer.normals.empty()){
            iLayer.normals.clear();
            for(const auto& iVertInfo : ins_aosVertices)
                iLayer.normals.emplace_back(iVertInfo.layeredNormal[idxLayer]);
        }

        if(!iLayer.binormals.empty()){
            iLayer.binormals.clear();
            for(const auto& iVertInfo : ins_aosVertices)
                iLayer.binormals.emplace_back(iVertInfo.layeredBinormal[idxLayer]);
        }

        if(!iLayer.tangents.empty()){
            iLayer.tangents.clear();
            for(const auto& iVertInfo : ins_aosVertices)
                iLayer.tangents.emplace_back(iVertInfo.layeredTangent[idxLayer]);
        }

        if(!iLayer.texcoords.table.empty()){
            iLayer.texcoords.table.clear();
            for(const auto& iVertInfo : ins_aosVertices)
                iLayer.texcoords.table.emplace_back(iVertInfo.layeredUV[idxLayer].second);
        }
    }
}

static inline void ins_removeUnusedDeforms(NodeData* pNodeData){
    if(pNodeData->bufSkinData.empty())
        return;

    ins_nodeUsageChecker.clear();
    for(const auto& iVert : pNodeData->bufSkinData){
        for(const auto& iWeight : iVert)
            ins_nodeUsageChecker.emplace(iWeight.cluster);
    }

    for(auto i = pNodeData->mapBoneDeformMatrices.begin(); i != pNodeData->mapBoneDeformMatrices.end();){
        if(ins_nodeUsageChecker.find(i->first) == ins_nodeUsageChecker.end())
            i = pNodeData->mapBoneDeformMatrices.erase(i);
        else
            ++i;
    }
}


void SHROptimizeMesh(NodeData* pNodeData){
    ins_fillAOSContainers(pNodeData);
    ins_genOptimizeMesh(pNodeData);
    ins_removeUnusedDeforms(pNodeData);
}
