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


class _VertexInfo{
public:
    static inline size_t makeHash(const _VertexInfo& data){
        size_t c, result = 2166136261U; //FNV1 hash. Perhaps the best string hash

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
            c = MakeHash(data.position.mData);
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
    inline bool operator==(const _VertexInfo& rhs)const{
        if(position != rhs.position)
            return false;

        if(skinData.size() != rhs.skinData.size())
            return false;
        for(size_t idx = 0, edx = skinData.size(); idx < edx; ++idx){
            const auto& lhsSkin = skinData[idx];
            const auto& rhsSkin = rhs.skinData[idx];
            if(lhsSkin.weight != rhsSkin.weight)
                return false;
            if(lhsSkin.cluster != rhsSkin.cluster)
                return false;
        }

        if(layeredColor.size() != rhs.layeredColor.size())
            return false;
        for(size_t idx = 0, edx = layeredColor.size(); idx < edx; ++idx){
            if(layeredColor[idx] != rhs.layeredColor[idx])
                return false;
        }

        if(layeredNormal.size() != rhs.layeredNormal.size())
            return false;
        for(size_t idx = 0, edx = layeredNormal.size(); idx < edx; ++idx){
            if(layeredNormal[idx] != rhs.layeredNormal[idx])
                return false;
        }

        if(layeredBinormal.size() != rhs.layeredBinormal.size())
            return false;
        for(size_t idx = 0, edx = layeredBinormal.size(); idx < edx; ++idx){
            if(layeredBinormal[idx] != rhs.layeredBinormal[idx])
                return false;
        }

        if(layeredTangent.size() != rhs.layeredTangent.size())
            return false;
        for(size_t idx = 0, edx = layeredTangent.size(); idx < edx; ++idx){
            if(layeredTangent[idx] != rhs.layeredTangent[idx])
                return false;
        }

        if(layeredUV.size() != rhs.layeredUV.size())
            return false;
        for(size_t idx = 0, edx = layeredUV.size(); idx < edx; ++idx){
            if(layeredUV[idx].second != rhs.layeredUV[idx].second)
                return false;
            if(layeredUV[idx].first != rhs.layeredUV[idx].first)
                return false;
        }

        return true;
    }


public:
    fbxsdk::FbxDouble3 position;

    eastl::vector<SkinInfo> skinData;

    Vector4Container layeredColor;

    Unit3Container layeredNormal;
    Unit3Container layeredBinormal;
    Unit3Container layeredTangent;

    eastl::vector<eastl::pair<eastl::string, fbxsdk::FbxDouble2>> layeredUV;
};
class _PolygonInfo{
public:
    inline bool useSameMaterial(const _PolygonInfo& rhs)const{
        if(layeredMaterial.size() != rhs.layeredMaterial.size())
            return false;
        for(size_t idx = 0, edx = layeredMaterial.size(); idx < edx; ++idx){
            if(layeredMaterial[idx] != rhs.layeredMaterial[idx])
                return false;
        }

        return true;
    }


public:
    Int3 indices;
    IntContainer layeredMaterial;
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

    inline bool operator==(const _VertexInfoKey& rhs)const{ return (data == rhs.data); }


private:
    const size_t hash;
    const _VertexInfo& data;
};


static eastl::vector<_VertexInfo> ins_aosVertices;
static eastl::vector<_PolygonInfo> ins_aosPolygons;

static eastl::unordered_map<_VertexInfoKey, int> ins_aosVertexFinder;
static eastl::vector<int> ins_flatVertexBinder;


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

    for(size_t idxPoly = 0, edxPoly = pNodeData->bufIndices.size(); idxPoly < edxPoly; ++idxPoly){
        const auto& iPoly = pNodeData->bufIndices[idxPoly];

        auto& iPolyInfo = ins_aosPolygons[idxPoly];

        for(size_t idxLayer = 0, edxLayer = pNodeData->bufLayers.size(); idxLayer < edxLayer; ++idxLayer){
            const auto& iLayer = pNodeData->bufLayers[idxLayer];

            if(!iLayer.materials.empty())
                iPolyInfo.layeredMaterial[idxLayer] = iLayer.materials[idxPoly];
            else
                iPolyInfo.layeredMaterial.clear();
        }

        for(size_t idxLocalVert = 0; idxLocalVert < 3; ++idxLocalVert){
            const auto idxVert = iPoly.raw[idxLocalVert];

            int idxVertInfo;
            {
                _VertexInfo iVertInfo;
                {
                    iVertInfo.position = pNodeData->bufPositions[idxVert];

                    iVertInfo.skinData = pNodeData->bufSkinData[idxVert];

                    size_t colorCount = 0;
                    size_t normalCount = 0;
                    size_t binormalCount = 0;
                    size_t tangentCount = 0;
                    size_t uvCount = 0;

                    for(size_t idxLayer = 0; idxLayer < layerCount; ++idxLayer){
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

                    for(size_t idxLayer = 0, edxLayer = iVertInfo.layeredColor.size(); idxLayer < edxLayer; ++idxLayer){
                        const auto& iLayer = pNodeData->bufLayers[idxLayer];

                        iVertInfo.layeredColor[idxLayer] = iLayer.colors[idxVert];
                    }
                    for(size_t idxLayer = 0, edxLayer = iVertInfo.layeredNormal.size(); idxLayer < edxLayer; ++idxLayer){
                        const auto& iLayer = pNodeData->bufLayers[idxLayer];

                        iVertInfo.layeredNormal[idxLayer] = iLayer.normals[idxVert];
                    }
                    for(size_t idxLayer = 0, edxLayer = iVertInfo.layeredBinormal.size(); idxLayer < edxLayer; ++idxLayer){
                        const auto& iLayer = pNodeData->bufLayers[idxLayer];

                        iVertInfo.layeredBinormal[idxLayer] = iLayer.binormals[idxVert];
                    }
                    for(size_t idxLayer = 0, edxLayer = iVertInfo.layeredTangent.size(); idxLayer < edxLayer; ++idxLayer){
                        const auto& iLayer = pNodeData->bufLayers[idxLayer];

                        iVertInfo.layeredTangent[idxLayer] = iLayer.tangents[idxVert];
                    }
                    for(size_t idxLayer = 0, edxLayer = iVertInfo.layeredUV.size(); idxLayer < edxLayer; ++idxLayer){
                        const auto& iLayer = pNodeData->bufLayers[idxLayer];

                        auto& v = iVertInfo.layeredUV[idxLayer];
                        v.first = iLayer.texcoords.name;
                        v.second = iLayer.texcoords.table[idxVert];
                    }
                }

                const auto iVertInfoHash = _VertexInfo::makeHash(iVertInfo);
                auto fVertexInfo = ins_aosVertexFinder.find(_VertexInfoKey(iVertInfo, iVertInfoHash));
                if(fVertexInfo == ins_aosVertexFinder.end()){
                    idxVertInfo = ins_aosVertices.size();
                    ins_aosVertices.emplace_back(eastl::move(iVertInfo));

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
    pNodeData->bufIndices.clear();
    for(const auto& iPolyInfo : ins_aosPolygons)
        pNodeData->bufIndices.emplace_back(iPolyInfo.indices);

    pNodeData->bufPositions.clear();
    pNodeData->bufSkinData.clear();
    for(const auto& iVertInfo : ins_aosVertices){
        pNodeData->bufPositions.emplace_back(iVertInfo.position);
        pNodeData->bufSkinData.emplace_back(eastl::move(iVertInfo.skinData));
    }

    for(size_t idxLayer = 0, edxLayer = pNodeData->bufLayers.size(); idxLayer < edxLayer; ++idxLayer){
        auto& iLayer = pNodeData->bufLayers[idxLayer];

        if(!iLayer.materials.empty()){
            iLayer.materials.clear();
            for(const auto& iPolyInfo : ins_aosPolygons)
                iLayer.materials.emplace_back(iPolyInfo.layeredMaterial[idxLayer]);
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


void SHROptimizeMesh(NodeData* pNodeData){
    ins_fillAOSContainers(pNodeData);
    ins_genOptimizeMesh(pNodeData);
}