/**
* @file FBXShared_BoneCombination.cpp
* @date 2020/05/18
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <eastl/map.h>
#include <eastl/unordered_set.h>

#include "FBXShared.h"


using _ClusterCountChecker = eastl::unordered_set<FbxCluster*>;

class _OrderedKey{
public:
    inline operator size_t()const{
        return MakeHash(layers.data(), layers.size());
    }


public:
    IntContainer layers;
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
        size_t key[] = { (size_t)(*static_cast<const _OrderedKey*>(this)), (size_t)index };
        return MakeHash(key);
    }


public:
    int index;
};

struct _MeshPolyValue{
    _ClusterCountChecker participatedClusters;
    IntContainer polyIndices;
};

using _TempMeshPolys = eastl::map<_OrderedKey, IntContainer>;
using _MeshPolys = eastl::multimap<_OrderedKey, _MeshPolyValue>;


static _TempMeshPolys ins_tmpMeshPolys;
static _MeshPolys ins_meshPolys;

//static _ClusterCountChecker ins_clusterCountChecker;
static _ClusterCountChecker ins_localClusterCounterChecker;

static eastl::unordered_map<_OrderdKeyWithIndex, int> ins_vertOldToNew;
static eastl::vector<int> ins_vertNewToOld;

static Vector3Container ins_bufPositions;
static Int3Container ins_bufIndices;

static eastl::vector<LayerElement> ins_bufLayers;

static SkinInfoContainer ins_bufSkinData;


static void ins_genTempMeshAttribute(NodeData* pNodeData){
    ins_tmpMeshPolys.clear();

    const auto edxLayer = pNodeData->bufLayers.size();
    if(!edxLayer){
        _OrderedKey newKey;

        IntContainer polyIndices;
        polyIndices.reserve(pNodeData->bufIndices.size());
        for(int idxPoly = 0, edxPoly = (int)pNodeData->bufIndices.size(); idxPoly < edxPoly; ++idxPoly)
            polyIndices.emplace_back(idxPoly);

        ins_tmpMeshPolys.emplace(eastl::move(newKey), eastl::move(polyIndices));
    }
    else{
        for(size_t idxPoly = 0, edxPoly = pNodeData->bufIndices.size(); idxPoly < edxPoly; ++idxPoly){
            _OrderedKey newKey;
            newKey.layers.reserve(edxLayer);

            for(size_t idxLayer = 0; idxLayer < edxLayer; ++idxLayer){
                const auto& iLayer = pNodeData->bufLayers[idxLayer];

                if(!iLayer.materials.empty())
                    newKey.layers[idxLayer] = iLayer.materials[idxPoly];
            }

            auto f = ins_tmpMeshPolys.find(newKey);
            if(f == ins_tmpMeshPolys.end()){
                IntContainer polyIndices;
                polyIndices.reserve(edxPoly);
                polyIndices.emplace_back(idxPoly);
                ins_tmpMeshPolys.emplace(eastl::move(newKey), eastl::move(polyIndices));
            }
            else
                f->second.emplace_back(idxPoly);
        }
    }
}

static void ins_genMeshAttribute(NodeData* pNodeData){
    ins_meshPolys.clear();
    for(const auto& iAttr : ins_tmpMeshPolys){
        _MeshPolyValue newPolyVal;
        newPolyVal.polyIndices = eastl::move(iAttr.second);

        ins_meshPolys.emplace(eastl::move(iAttr.first), eastl::move(newPolyVal));
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

            itrCurPoly = ins_meshPolys.emplace(iAttr.first, eastl::move(reservePolyVal));
        }

        for(const auto& idxPoly : iAttr.second){
            const auto& iPoly = pNodeData->bufIndices[idxPoly];

            ins_localClusterCounterChecker.clear();

            for(const auto& idxVert : iPoly.raw){
                const auto& iSkin = pNodeData->bufSkinData[idxVert];
                for(const auto& iWeight : iSkin){
                    itrCurPoly->second.participatedClusters.emplace(iWeight.cluster);
                    ins_localClusterCounterChecker.emplace(iWeight.cluster);
                }
            }

            if(itrCurPoly->second.participatedClusters.size() <= shr_ioSetting.MaxBoneCountPerMesh)
                itrCurPoly->second.polyIndices.emplace_back(idxPoly);
            else{
                _MeshPolyValue newPolyVal;
                newPolyVal.polyIndices.reserve(iAttr.second.size());
                newPolyVal.participatedClusters.rehash(pNodeData->mapBoneDeformMatrices.size() << 1);
                newPolyVal.polyIndices.emplace_back(idxPoly);

                itrCurPoly = ins_meshPolys.emplace(iAttr.first, eastl::move(newPolyVal));

                eastl::swap(itrCurPoly->second.participatedClusters, ins_localClusterCounterChecker);
            }
        }
    }
}

static void ins_rearrangeMesh(NodeData* pNodeData){
    pNodeData->bufMeshAttribute.clear();
    pNodeData->bufMeshAttribute.reserve(ins_meshPolys.size());

    pNodeData->bufBoneCombination.clear();
    pNodeData->bufBoneCombination.reserve(ins_meshPolys.size());

    for(const auto& iAttr : ins_meshPolys){
        MeshAttributeElement meshAttribute;
        meshAttribute.PolygonFirst = ins_bufIndices.size();
        meshAttribute.VertexFirst = ins_vertNewToOld.size();

        _OrderdKeyWithIndex newOrderKey;
        newOrderKey.layers = iAttr.first.layers;

        for(const auto& idxOldPoly : iAttr.second.polyIndices){
            const auto& iOldPoly = pNodeData->bufIndices[idxOldPoly];
            Int3 iNewPoly;

            for(size_t idxVert = 0; idxVert < 3; ++idxVert){
                const auto& idxOldVert = iOldPoly.raw[idxVert];
                auto& idxNewVert = iNewPoly.raw[idxVert];

                newOrderKey.index = idxOldVert;
                auto f = ins_vertOldToNew.find(newOrderKey);
                if(f == ins_vertOldToNew.end()){
                    idxNewVert = (int)ins_vertNewToOld.size();
                    ins_vertNewToOld.emplace_back(idxOldVert);
                    ins_vertOldToNew.emplace(newOrderKey, idxNewVert);
                }
                else
                    idxNewVert = f->second;
            }

            ins_bufIndices.emplace_back(eastl::move(iNewPoly));

            if(!iAttr.first.layers.empty()){
                for(size_t idxLayer = 0, edxLayer = pNodeData->bufLayers.size(); idxLayer < edxLayer; ++idxLayer){
                    const auto& idxOldMaterial = iAttr.first.layers[idxLayer];
                    auto& iNewMaterial = ins_bufLayers[idxLayer].materials;

                    iNewMaterial.emplace_back(idxOldMaterial);
                }
            }
        }

        meshAttribute.PolygonLast = ins_bufIndices.size() - 1u;
        meshAttribute.VertexLast = ins_vertNewToOld.size() - 1u;

        pNodeData->bufMeshAttribute.emplace_back(eastl::move(meshAttribute));
    }

    for(const auto& idxOldVert : ins_vertNewToOld)
        ins_bufPositions.emplace_back(pNodeData->bufPositions[idxOldVert]);

    for(size_t idxLayer = 0, edxLayer = pNodeData->bufLayers.size(); idxLayer < edxLayer; ++idxLayer){
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

            pNodeData->bufBoneCombination.emplace_back(eastl::move(boneCombination));
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
        for(size_t idxLayer = 0, edxLayer = ins_bufLayers.size(); idxLayer < edxLayer; ++idxLayer){
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

            lhsLayer.texcoords.name = eastl::move(rhsLayer.texcoords.name);
            lhsLayer.texcoords.table.clear();
            lhsLayer.texcoords.table.reserve(vertReserveSize);
        }

        ins_bufSkinData.clear();
        ins_bufSkinData.reserve(vertReserveSize);
    }

    ins_rearrangeMesh(pNodeData);

    {
        eastl::swap(pNodeData->bufPositions, ins_bufPositions);
        eastl::swap(pNodeData->bufIndices, ins_bufIndices);
        eastl::swap(pNodeData->bufLayers, ins_bufLayers);
        eastl::swap(pNodeData->bufSkinData, ins_bufSkinData);
    }
}