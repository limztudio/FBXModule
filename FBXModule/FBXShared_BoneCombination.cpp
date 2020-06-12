/**
* @file FBXShared_BoneCombination.cpp
* @date 2020/05/18
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <map>
#include <unordered_set>

#include "FBXShared.h"


using _ClusterCountChecker = std::unordered_set<FbxCluster*, PointerHasher<FbxCluster*>>;

class _OrderedKey{
public:
    inline operator size_t()const{
        return MakeHash(layers.data(), layers.size());
    }


public:
    UintContainer layers;
};
static inline bool operator<(const _OrderedKey& lhs, const _OrderedKey& rhs){
    const auto cntLhs = lhs.layers.size();
    const auto cntRhs = rhs.layers.size();

    if(cntLhs < cntRhs)
        return true;
    else if(cntLhs > cntRhs)
        return false;

    for(size_t idx = 0; idx < cntLhs; ++idx){
        if(lhs.layers[idx] < rhs.layers[idx])
            return true;
    }

    return false;
}
class _OrderdKeyWithIndex : public _OrderedKey{
public:
    inline operator size_t()const{
        size_t key[] = { (size_t)(*static_cast<const _OrderedKey*>(this)), (size_t)index, attribute };
        return MakeHash(key);
    }


public:
    unsigned int index;
    size_t attribute;
};

struct _MeshPolyValue{
    _ClusterCountChecker participatedClusters;
    UintContainer polyIndices;
};

using _TempMeshPolys = std::map<_OrderedKey, UintContainer>;
using _MeshPolys = std::multimap<_OrderedKey, _MeshPolyValue>;


static _TempMeshPolys ins_tmpMeshPolys;
static _MeshPolys ins_meshPolys;

static _ClusterCountChecker ins_localClusterCounterChecker[2];

static std::unordered_map<_OrderdKeyWithIndex, unsigned int, CustomHasher<_OrderdKeyWithIndex>> ins_vertOldToNew;
static std::vector<unsigned int> ins_vertNewToOld;

static Vector3Container ins_bufPositions;
static Uint3Container ins_bufIndices;

static std::vector<LayerElement> ins_bufLayers;

static SkinInfoContainer ins_bufSkinData;


static void ins_genTempMeshAttribute(const NodeData* pNodeData){
    ins_tmpMeshPolys.clear();

    const auto edxLayer = pNodeData->bufLayers.size();
    if(!edxLayer){
        _OrderedKey newKey;

        UintContainer polyIndices;
        polyIndices.reserve(pNodeData->bufIndices.size());
        for(auto edxPoly = (unsigned int)pNodeData->bufIndices.size(), idxPoly = 0u; idxPoly < edxPoly; ++idxPoly)
            polyIndices.emplace_back(idxPoly);

        ins_tmpMeshPolys.emplace(std::move(newKey), std::move(polyIndices));
    }
    else{
        for(auto edxPoly = (unsigned int)pNodeData->bufIndices.size(), idxPoly = 0u; idxPoly < edxPoly; ++idxPoly){
            _OrderedKey newKey;
            newKey.layers.resize(edxLayer);

            for(size_t idxLayer = 0; idxLayer < edxLayer; ++idxLayer){
                const auto& iLayer = pNodeData->bufLayers[idxLayer];

                if(!iLayer.materials.empty())
                    newKey.layers[idxLayer] = iLayer.materials[idxPoly];
            }

            auto f = ins_tmpMeshPolys.find(newKey);
            if(f == ins_tmpMeshPolys.end()){
                UintContainer polyIndices;
                polyIndices.reserve(edxPoly);
                polyIndices.emplace_back((unsigned int)idxPoly);
                ins_tmpMeshPolys.emplace(std::move(newKey), std::move(polyIndices));
            }
            else
                f->second.emplace_back((unsigned int)idxPoly);
        }
    }
}

static void ins_genMeshAttribute(NodeData* pNodeData){
    ins_meshPolys.clear();
    for(const auto& iAttr : ins_tmpMeshPolys){
        _MeshPolyValue newPolyVal;
        newPolyVal.polyIndices = std::move(iAttr.second);

        ins_meshPolys.emplace(std::move(iAttr.first), std::move(newPolyVal));
    }
}
static void ins_genSkinnedMeshAttribute(NodeData* pNodeData){
    ins_meshPolys.clear();
    for(const auto& iAttr : ins_tmpMeshPolys){
        _MeshPolys::iterator itrCurPoly;
        {
            _MeshPolyValue reservePolyVal;
            reservePolyVal.polyIndices.reserve(iAttr.second.size());
            reservePolyVal.participatedClusters.rehash(pNodeData->mapBoneDeformMatrices.size() << 1);

            itrCurPoly = ins_meshPolys.emplace(iAttr.first, std::move(reservePolyVal));
        }

        for(const auto& idxPoly : iAttr.second){
            const auto& iPoly = pNodeData->bufIndices[idxPoly];

            ins_localClusterCounterChecker[0] = itrCurPoly->second.participatedClusters;
            ins_localClusterCounterChecker[1].clear();

            for(const auto& idxVert : iPoly.raw){
                const auto& iSkin = pNodeData->bufSkinData[idxVert];
                for(const auto& iWeight : iSkin){
                    itrCurPoly->second.participatedClusters.emplace(iWeight.cluster);
                    ins_localClusterCounterChecker[1].emplace(iWeight.cluster);
                }
            }

            if(itrCurPoly->second.participatedClusters.size() <= shr_ioSetting.MaxBoneCountPerMesh)
                itrCurPoly->second.polyIndices.emplace_back(idxPoly);
            else{
                std::swap(itrCurPoly->second.participatedClusters, ins_localClusterCounterChecker[0]);

                _MeshPolyValue newPolyVal;
                newPolyVal.polyIndices.reserve(iAttr.second.size());
                newPolyVal.participatedClusters.rehash(pNodeData->mapBoneDeformMatrices.size() << 1);
                newPolyVal.polyIndices.emplace_back(idxPoly);

                itrCurPoly = ins_meshPolys.emplace(iAttr.first, std::move(newPolyVal));

                std::swap(itrCurPoly->second.participatedClusters, ins_localClusterCounterChecker[1]);
            }
        }
    }
}

static void ins_rearrangeMesh(NodeData* pNodeData){
    pNodeData->bufMeshAttribute.clear();
    pNodeData->bufMeshAttribute.reserve(ins_meshPolys.size());

    pNodeData->bufBoneCombination.clear();
    pNodeData->bufBoneCombination.reserve(ins_meshPolys.size());

    size_t idxAttr = 0u;
    for(auto itAttr = ins_meshPolys.begin(), etAttr = ins_meshPolys.end(); itAttr != etAttr; ++itAttr, ++idxAttr){
        const auto& iAttr = *itAttr;
        MeshAttributeElement meshAttribute;
        meshAttribute.PolygonFirst = decltype(meshAttribute.PolygonFirst)(ins_bufIndices.size());
        meshAttribute.VertexFirst = decltype(meshAttribute.VertexFirst)(ins_vertNewToOld.size());

        _OrderdKeyWithIndex newOrderKey;
        newOrderKey.layers = iAttr.first.layers;
        newOrderKey.attribute = idxAttr;

        for(const auto& idxOldPoly : iAttr.second.polyIndices){
            const auto& iOldPoly = pNodeData->bufIndices[idxOldPoly];
            Uint3 iNewPoly;

            for(size_t idxVert = 0u; idxVert < 3u; ++idxVert){
                const auto& idxOldVert = iOldPoly.raw[idxVert];
                auto& idxNewVert = iNewPoly.raw[idxVert];

                newOrderKey.index = idxOldVert;
                auto f = ins_vertOldToNew.find(newOrderKey);
                if(f == ins_vertOldToNew.end()){
                    idxNewVert = (unsigned int)ins_vertNewToOld.size();
                    ins_vertNewToOld.emplace_back(idxOldVert);
                    ins_vertOldToNew.emplace(newOrderKey, idxNewVert);
                }
                else
                    idxNewVert = f->second;
            }

            ins_bufIndices.emplace_back(std::move(iNewPoly));

            if(!iAttr.first.layers.empty()){
                for(auto edxLayer = (unsigned int)pNodeData->bufLayers.size(), idxLayer = 0u; idxLayer < edxLayer; ++idxLayer){
                    const auto& iOldMaterial = pNodeData->bufLayers[idxLayer].materials;
                    if(!iOldMaterial.empty()){
                        const auto& idxOldMaterial = iAttr.first.layers[idxLayer];
                        auto& iNewMaterial = ins_bufLayers[idxLayer].materials;

                        iNewMaterial.emplace_back(idxOldMaterial);
                    }
                }
            }
        }

        meshAttribute.PolygonLast = decltype(meshAttribute.PolygonLast)(ins_bufIndices.size() - 1u);
        meshAttribute.VertexLast = decltype(meshAttribute.VertexLast)(ins_vertNewToOld.size() - 1u);

        pNodeData->bufMeshAttribute.emplace_back(std::move(meshAttribute));
    }

    for(const auto& idxOldVert : ins_vertNewToOld)
        ins_bufPositions.emplace_back(pNodeData->bufPositions[idxOldVert]);

    for(auto edxLayer = (unsigned int)pNodeData->bufLayers.size(), idxLayer = 0u; idxLayer < edxLayer; ++idxLayer){
        const auto& iOldLayer = pNodeData->bufLayers[idxLayer];
        auto& iNewLayer = ins_bufLayers[idxLayer];

        if(!iOldLayer.colors.empty()){
            for(const auto& idxOldVert : ins_vertNewToOld)
                iNewLayer.colors.emplace_back(iOldLayer.colors[idxOldVert]);
        }
        if(!iOldLayer.normals.empty()){
            for(const auto& idxOldVert : ins_vertNewToOld)
                iNewLayer.normals.emplace_back(iOldLayer.normals[idxOldVert]);
        }
        if(!iOldLayer.binormals.empty()){
            for(const auto& idxOldVert : ins_vertNewToOld)
                iNewLayer.binormals.emplace_back(iOldLayer.binormals[idxOldVert]);
        }
        if(!iOldLayer.tangents.empty()){
            for(const auto& idxOldVert : ins_vertNewToOld)
                iNewLayer.tangents.emplace_back(iOldLayer.tangents[idxOldVert]);
        }
        if(!iOldLayer.texcoords.table.empty()){
            for(const auto& idxOldVert : ins_vertNewToOld)
                iNewLayer.texcoords.table.emplace_back(iOldLayer.texcoords.table[idxOldVert]);
        }
    }

    if(!pNodeData->bufSkinData.empty()){
        for(const auto& iAttr : ins_meshPolys){
            BoneCombination boneCombination;
            boneCombination.reserve(iAttr.second.participatedClusters.size());
            for(auto* iCluster : iAttr.second.participatedClusters)
                boneCombination.emplace_back(iCluster);

            pNodeData->bufBoneCombination.emplace_back(std::move(boneCombination));
        }

        for(const auto& idxOldVert : ins_vertNewToOld)
            ins_bufSkinData.emplace_back(pNodeData->bufSkinData[idxOldVert]);
    }
}


void SHRGenerateMeshAttribute(NodeData* pNodeData){
    ins_genTempMeshAttribute(pNodeData);

    if(pNodeData->bufSkinData.empty())
        ins_genMeshAttribute(pNodeData);
    else
        ins_genSkinnedMeshAttribute(pNodeData);

    {
        const auto vertReserveSize = pNodeData->bufPositions.size() << 1;
        const auto indReserveSize = pNodeData->bufIndices.size();

        ins_vertOldToNew.clear();
        ins_vertOldToNew.rehash(vertReserveSize << 1);

        ins_vertNewToOld.clear();
        ins_vertNewToOld.reserve(vertReserveSize);

        ins_bufPositions.clear();
        ins_bufPositions.reserve(vertReserveSize);

        ins_bufIndices.clear();
        ins_bufIndices.reserve(indReserveSize);

        ins_bufLayers.clear();
        ins_bufLayers.resize(pNodeData->bufLayers.size());
        for(auto edxLayer = (unsigned int)ins_bufLayers.size(), idxLayer = 0u; idxLayer < edxLayer; ++idxLayer){
            auto& lhsLayer = ins_bufLayers[idxLayer];
            const auto& rhsLayer = pNodeData->bufLayers[idxLayer];

            lhsLayer.materials.clear();
            lhsLayer.materials.reserve(indReserveSize);

            lhsLayer.colors.clear();
            lhsLayer.colors.reserve(vertReserveSize);

            lhsLayer.normals.clear();
            lhsLayer.normals.reserve(vertReserveSize);

            lhsLayer.binormals.clear();
            lhsLayer.binormals.reserve(vertReserveSize);

            lhsLayer.tangents.clear();
            lhsLayer.tangents.reserve(vertReserveSize);

            lhsLayer.texcoords.name = std::move(rhsLayer.texcoords.name);
            lhsLayer.texcoords.table.clear();
            lhsLayer.texcoords.table.reserve(vertReserveSize);
        }

        ins_bufSkinData.clear();
        ins_bufSkinData.reserve(vertReserveSize);
    }

    ins_rearrangeMesh(pNodeData);

    {
        std::swap(pNodeData->bufPositions, ins_bufPositions);
        std::swap(pNodeData->bufIndices, ins_bufIndices);
        std::swap(pNodeData->bufLayers, ins_bufLayers);
        std::swap(pNodeData->bufSkinData, ins_bufSkinData);
    }
}
