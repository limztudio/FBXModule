/**
 * @file FBXModule_Copy.cpp
 * @date 2018/06/15
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#include "stdafx.h"

#include "FBXShared.h"


using namespace fbxsdk;


__FBXM_MAKE_FUNC(void, __hidden_FBXModule_RebindRoot, void* pDest, const void* pSrc){
    auto* dest = static_cast<FBXRoot*>(pDest);
    const auto* src = static_cast<const FBXRoot*>(pSrc);

    SHRRebindRoot(dest, src);
}

__FBXM_MAKE_FUNC(bool, __hidden_FBXModule_CollapseMesh, void** pDest, const void* pSrc, const void** pOldNodeList, const void** pNewerNodeList, unsigned long nodeCount){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("FBXCollapseBone(FBXRoot*, FBXSkinnedMesh**, const FBXNode**, const FBXNode**, unsigned long)");


    const auto* pOldMesh = reinterpret_cast<const FBXSkinnedMesh*>(pSrc);
    fbx_unordered_map<FBXNode*, FBXNode*, PointerHasher<FBXNode*>> nodeConverter;
    {
        const auto** pConvOldList = reinterpret_cast<const FBXNode**>(pOldNodeList);
        const auto** pConvNewList = reinterpret_cast<const FBXNode**>(pNewerNodeList);

        fbx_unordered_set<FBXNode*, PointerHasher<FBXNode*>> collapseNodes;

        collapseNodes.rehash(nodeCount << 1);
        nodeConverter.rehash(nodeCount << 1);

        for(size_t idxNode = 0u; idxNode < nodeCount; ++idxNode){
            auto* pCurOld = const_cast<FBXNode*>(pConvOldList[idxNode]);
            auto* pCurNew = const_cast<FBXNode*>(pConvNewList[idxNode]);

            collapseNodes.emplace(pCurNew);
            nodeConverter.emplace(pCurOld, pCurNew);
        }

        {
            fbx_unordered_set<FBXNode*, PointerHasher<FBXNode*>> skinNodes;

            FBX_SIZE edxDeform = pOldMesh->SkinDeforms.Length;
            skinNodes.rehash(edxDeform << 1);
            for(FBX_SIZE idxDeform = 0u; idxDeform < edxDeform; ++idxDeform)
                skinNodes.emplace(pOldMesh->SkinDeforms.Values[idxDeform].TargetNode);

            for(auto* pNode : collapseNodes){
                if(skinNodes.find(pNode) == skinNodes.cend()){
                    SHRPushErrorMessage(FBX_TEXT("Collapsed node must be one of the clusters of collapse mesh"), __name_of_this_func);
                    return false;
                }
            }
        }
    }

    auto* pNewMesh = FBXNew<FBXSkinnedMesh>();

    NodeData genNodeData;
    {
        genNodeData.strName = ConvertString<char>(pOldMesh->Name.Values);
        CopyArrayData<pOldMesh->TransformMatrix.Length>((double*)genNodeData.kTransformMatrix, pOldMesh->TransformMatrix.Values);

        genNodeData.bufMaterials.resize(pOldMesh->Materials.Length);
        CopyArrayData(genNodeData.bufMaterials.data(), pOldMesh->Materials.Values, pOldMesh->Materials.Length);

        genNodeData.bufPositions.resize(pOldMesh->Vertices.Length);
        for(size_t idxElem = 0u; idxElem < pOldMesh->Vertices.Length; ++idxElem)
            CopyArrayData(genNodeData.bufPositions[idxElem].mData, pOldMesh->Vertices.Values[idxElem].Values);

        genNodeData.bufIndices.resize(pOldMesh->Indices.Length);
        for(size_t idxElem = 0u; idxElem < pOldMesh->Indices.Length; ++idxElem)
            CopyArrayData(genNodeData.bufIndices[idxElem].raw, pOldMesh->Indices.Values[idxElem].Values);

        genNodeData.bufLayers.resize(pOldMesh->LayeredElements.Length);
        for(size_t idxLayer = 0u; idxLayer < pOldMesh->LayeredElements.Length; ++idxLayer){
            auto& cGenLayer = genNodeData.bufLayers[idxLayer];
            const auto& cOldLayer = pOldMesh->LayeredElements.Values[idxLayer];

            if(cOldLayer.Material.Length){
                cGenLayer.materials.resize(pOldMesh->Indices.Length);
                for(size_t idxAttr = 0u; idxAttr < pOldMesh->Attributes.Length; ++idxAttr){
                    const auto& cMat = cOldLayer.Material.Values[idxAttr];
                    const auto& cAttr = pOldMesh->Attributes.Values[idxAttr];

                    for(size_t idxElem = cAttr.IndexStart; idxElem < (cAttr.IndexStart + cAttr.IndexCount); ++idxElem)
                        cGenLayer.materials[idxElem] = (unsigned int)(cMat);
                }
            }

            cGenLayer.colors.resize(cOldLayer.Color.Length);
            for(size_t idxElem = 0u; idxElem < cOldLayer.Color.Length; ++idxElem)
                CopyArrayData(cGenLayer.colors[idxElem].mData, cOldLayer.Color.Values->Values);

            cGenLayer.normals.resize(cOldLayer.Normal.Length);
            for(size_t idxElem = 0u; idxElem < cOldLayer.Normal.Length; ++idxElem)
                CopyArrayData(cGenLayer.normals[idxElem].mData, cOldLayer.Normal.Values->Values);

            cGenLayer.binormals.resize(cOldLayer.Binormal.Length);
            for(size_t idxElem = 0u; idxElem < cOldLayer.Binormal.Length; ++idxElem)
                CopyArrayData(cGenLayer.binormals[idxElem].mData, cOldLayer.Binormal.Values->Values);

            cGenLayer.tangents.resize(cOldLayer.Tangent.Length);
            for(size_t idxElem = 0u; idxElem < cOldLayer.Tangent.Length; ++idxElem)
                CopyArrayData(cGenLayer.tangents[idxElem].mData, cOldLayer.Tangent.Values->Values);

            cGenLayer.texcoords.name = fbx_to_string(idxLayer);
            cGenLayer.texcoords.table.resize(cOldLayer.Texcoord.Length);
            for(size_t idxElem = 0u; idxElem < cOldLayer.Texcoord.Length; ++idxElem)
                CopyArrayData(cGenLayer.texcoords.table[idxElem].mData, cOldLayer.Texcoord.Values->Values);
        }

        genNodeData.bufSkinData.resize(genNodeData.bufPositions.size());
        for(size_t idxSkin = 0u; idxSkin < pOldMesh->SkinInfos.Length; ++idxSkin){
            auto& cGenSkin = genNodeData.bufSkinData[idxSkin];
            const auto& cOldSkin = pOldMesh->SkinInfos.Values[idxSkin];

            cGenSkin.resize(cOldSkin.Length);
            for(size_t idxElem = 0u; idxElem < cOldSkin.Length; ++idxElem){
                auto& cGenElem = cGenSkin[idxElem];
                const auto& cOldElem = cOldSkin.Values[idxElem];

                auto* pBindNode = cOldElem.BindNode;
                {
                    auto f = nodeConverter.find(cOldElem.BindNode);
                    if(f != nodeConverter.cend())
                        pBindNode = f->second;
                }

                cGenElem.weight = decltype(cGenElem.weight)(cOldElem.Weight);
                cGenElem.cluster = reinterpret_cast<decltype(cGenElem.cluster)>(pBindNode);
            }
        }

        for(size_t idxDeform = 0u; idxDeform < pOldMesh->SkinDeforms.Length; ++idxDeform){
            const auto& cOldDeform = pOldMesh->SkinDeforms.Values[idxDeform];

            auto* pTargetNode = cOldDeform.TargetNode;
            {
                auto f = nodeConverter.find(cOldDeform.TargetNode);
                if(f != nodeConverter.cend()){
                    if(pTargetNode != f->second)
                        continue;
                }
                else
                    continue;
            }

            std::pair<FbxAMatrix, FbxAMatrix> cGenMatrix;
            {
                CopyArrayData<cOldDeform.TransformMatrix.Length>((double*)cGenMatrix.first, cOldDeform.TransformMatrix.Values);
                CopyArrayData<cOldDeform.LinkMatrix.Length>((double*)cGenMatrix.second, cOldDeform.LinkMatrix.Values);
            }

            genNodeData.mapBoneDeformMatrices.emplace(reinterpret_cast<FbxCluster*>(pTargetNode), std::move(cGenMatrix));
        }
    }

    {
        // casting FBXNode to fbxsdk::FbxCluster or FBXNode to fbxsdk::FbxNode is not so proper for the future manipulation.
        // but the following two functions will reference only their address.

        SHROptimizeMesh(&genNodeData);
        SHRGenerateMeshAttribute(&genNodeData);
    }

    {
        pNewMesh->Name = pOldMesh->Name;
        pNewMesh->TransformMatrix = pOldMesh->TransformMatrix;

        pNewMesh->Attributes.Assign(genNodeData.bufMeshAttribute.size());
        for(size_t idxAttr = 0u; idxAttr < pNewMesh->Attributes.Length; ++idxAttr){
            const auto& iOldAttr = genNodeData.bufMeshAttribute[idxAttr];
            auto& iNewAttr = pNewMesh->Attributes.Values[idxAttr];

            iNewAttr.VertexStart = iOldAttr.VertexFirst;
            iNewAttr.IndexStart = iOldAttr.PolygonFirst;

            iNewAttr.VertexCount = 1 + iOldAttr.VertexLast - iOldAttr.VertexFirst;
            iNewAttr.IndexCount = 1 + iOldAttr.PolygonLast - iOldAttr.PolygonFirst;
        }

        pNewMesh->Indices.Assign(genNodeData.bufIndices.size());
        for(size_t idxInd = 0u; idxInd < pNewMesh->Indices.Length; ++idxInd){
            auto& iInd = pNewMesh->Indices.Values[idxInd];

            CopyArrayData(iInd.Values, genNodeData.bufIndices[idxInd].raw);
        }

        pNewMesh->Vertices.Assign(genNodeData.bufPositions.size());
        for(size_t idxVert = 0u; idxVert < pNewMesh->Vertices.Length; ++idxVert){
            auto& iVert = pNewMesh->Vertices.Values[idxVert];

            CopyArrayData(iVert.Values, genNodeData.bufPositions[idxVert].mData);
        }

        pNewMesh->LayeredElements.Assign(genNodeData.bufLayers.size());
        for(size_t idxLayer = 0u; idxLayer < pNewMesh->LayeredElements.Length; ++idxLayer){
            auto& iLayer = pNewMesh->LayeredElements.Values[idxLayer];
            const auto& nodeLayer = genNodeData.bufLayers[idxLayer];

            {
                auto& iObject = iLayer.Material;
                const auto& nodeObject = nodeLayer.materials;

                if(nodeObject.empty())
                    iObject.Assign(0u);
                else{
                    iObject.Assign(genNodeData.bufMeshAttribute.size());
                    for(size_t idxMat = 0u; idxMat < iObject.Length; ++idxMat){
                        const auto idxOldMat = pNewMesh->Attributes.Values[idxMat].IndexStart;
                        iObject.Values[idxMat] = nodeObject[idxOldMat];
                    }
                }
            }

            {
                auto& iObject = iLayer.Color;
                const auto& nodeObject = nodeLayer.colors;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0u; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }

            {
                auto& iObject = iLayer.Normal;
                const auto& nodeObject = nodeLayer.normals;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0u; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }

            {
                auto& iObject = iLayer.Binormal;
                const auto& nodeObject = nodeLayer.binormals;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0u; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }

            {
                auto& iObject = iLayer.Tangent;
                const auto& nodeObject = nodeLayer.tangents;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0u; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }

            {
                auto& iObject = iLayer.Texcoord;
                const auto& nodeObject = nodeLayer.texcoords.table;

                iObject.Assign(nodeObject.size());
                for(size_t idxElem = 0u; idxElem < iObject.Length; ++idxElem)
                    CopyArrayData(iObject.Values[idxElem].Values, nodeObject[idxElem].mData);
            }
        }

        pNewMesh->Materials.Assign(genNodeData.bufMaterials.size());
        for(size_t idxMaterial = 0u; idxMaterial < pNewMesh->Materials.Length; ++idxMaterial){
            auto& iMaterial = pNewMesh->Materials.Values[idxMaterial];
            const auto& nodeMaterial = genNodeData.bufMaterials[idxMaterial];

            iMaterial = nodeMaterial;
        }
    }

    {
        pNewMesh->BoneCombinations.Assign(genNodeData.bufBoneCombination.size());
        for(size_t idxAttr = 0u; idxAttr < pNewMesh->BoneCombinations.Length; ++idxAttr){
            auto& iAttr = pNewMesh->BoneCombinations.Values[idxAttr];
            const auto& nodeAttr = genNodeData.bufBoneCombination[idxAttr];

            iAttr.Assign(nodeAttr.size());
            for(size_t idxBC = 0u; idxBC < iAttr.Length; ++idxBC){
                auto*& iCluster = iAttr.Values[idxBC];
                auto* nodeCluster = nodeAttr[idxBC];

                iCluster = reinterpret_cast<decltype(iCluster)>(nodeCluster);
            }
        }

        pNewMesh->SkinInfos.Assign(genNodeData.bufSkinData.size());
        for(size_t idxSkin = 0u; idxSkin < pNewMesh->SkinInfos.Length; ++idxSkin){
            auto& iSkin = pNewMesh->SkinInfos.Values[idxSkin];
            const auto& nodeSkin = genNodeData.bufSkinData[idxSkin];

            iSkin.Assign(nodeSkin.size());
            for(size_t idxCluster = 0u; idxCluster < iSkin.Length; ++idxCluster){
                auto& iCluster = iSkin.Values[idxCluster];
                const auto& nodeCluster = nodeSkin[idxCluster];

                iCluster.BindNode = reinterpret_cast<decltype(iCluster.BindNode)>(nodeCluster.cluster);
                iCluster.Weight = static_cast<decltype(iCluster.Weight)>(nodeCluster.weight);
            }
        }

        pNewMesh->SkinDeforms.Assign(genNodeData.mapBoneDeformMatrices.size());
        auto* iDeform = pNewMesh->SkinDeforms.Values;
        for(const auto& nodeDeform : genNodeData.mapBoneDeformMatrices){
            auto* kBindNode = nodeDeform.first;

            iDeform->TargetNode = reinterpret_cast<decltype(iDeform->TargetNode)>(kBindNode);

            CopyArrayData(iDeform->TransformMatrix.Values, (const double*)nodeDeform.second.first);
            CopyArrayData(iDeform->LinkMatrix.Values, (const double*)nodeDeform.second.second);

            ++iDeform;
        }
    }

    (*pDest) = pNewMesh;
    return true;
}
