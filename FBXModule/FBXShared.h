/**
 * @file FBXUtilites.h
 * @date 2018/06/15
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#pragma once


#include <fbxsdk.h>

#include "FBXUtilites.h"

#include <FBXModule.hpp>


// structures
// Common ////////////////////////////////////////////////////////////////////////////////////////////

using UintContainer = fbx_vector<unsigned int>;
using IntContainer = fbx_vector<int>;
using Uint3Container = fbx_vector<Uint3>;
using Int3Container = fbx_vector<Int3>;
using Vector3Container = fbx_vector<fbxsdk::FbxDouble3>;
using Vector4Container = fbx_vector<fbxsdk::FbxDouble4>;
using Unit3Container = fbx_vector<fbxsdk::FbxDouble3>;

// FBXShared_Error ///////////////////////////////////////////////////////////////////////////////////

// FBXShared_Copy ///////////////////////////////////////////////////////////////////////////////////

// FBXShared_FbxSdk //////////////////////////////////////////////////////////////////////////////////

// FBXShared_Converter ///////////////////////////////////////////////////////////////////////////////

// FBXShared_Bone ////////////////////////////////////////////////////////////////////////////////////

// FBXShared_Material ////////////////////////////////////////////////////////////////////////////////

class MaterialTable{
public:
    inline void clear(){
        matTable.clear();
        matFinder.clear();
    }

    inline unsigned int emplace(fbxsdk::FbxSurfaceMaterial* kMaterial){
        unsigned int ret;

        auto f = matFinder.find(kMaterial);
        if(f == matFinder.end()){
            ret = decltype(ret)(matTable.size());

            matTable.emplace_back(kMaterial);
            matFinder.emplace(kMaterial, ret);
        }
        else
            ret = f->second;
        
        return ret;
    }

public:
    inline const fbx_vector<fbxsdk::FbxSurfaceMaterial*>& getTable()const{ return matTable; }


private:
    fbx_vector<fbxsdk::FbxSurfaceMaterial*> matTable;
    fbx_unordered_map<fbxsdk::FbxSurfaceMaterial*, unsigned int, PointerHasher<fbxsdk::FbxSurfaceMaterial*>> matFinder;
};

// FBXShared_Mesh ////////////////////////////////////////////////////////////////////////////////////

struct TexcoordTable{
    fbx_basic_string<char> name;
    fbx_vector<fbxsdk::FbxDouble2> table;
};

using ControlPointRemap = fbx_vector<fbx_unordered_set<unsigned int>>;

struct LayerElement{
    UintContainer materials;

    Vector4Container colors;
    Unit3Container normals;
    Unit3Container binormals;
    Unit3Container tangents;
    TexcoordTable texcoords;
};

using ControlPointMergeMap = UintContainer;

// FBXShared_Skin ////////////////////////////////////////////////////////////////////////////////////

struct SkinInfo{
    fbxsdk::FbxCluster* cluster;
    fbxsdk::FbxDouble weight;
};
struct SkinData{
    fbxsdk::FbxAMatrix matrix;
    fbxsdk::FbxDouble weight;
};

using SkinInfoContainer = fbx_vector<fbx_vector<SkinInfo>>;

using BoneOffsetMatrixMap = fbx_unordered_map<fbxsdk::FbxCluster*, std::pair<fbxsdk::FbxAMatrix, fbxsdk::FbxAMatrix>, PointerHasher<fbxsdk::FbxCluster*>>;

// FBXShared_BoneCombination /////////////////////////////////////////////////////////////////////////

struct MeshAttributeElement{
    unsigned long PolygonFirst;
    unsigned long PolygonLast;

    unsigned long VertexFirst;
    unsigned long VertexLast;
};

using MeshAttribute = fbx_vector<MeshAttributeElement>;
using BoneCombination = fbx_vector<fbxsdk::FbxCluster*>;

// FBXShared_Node ////////////////////////////////////////////////////////////////////////////////////

struct NodeData{
    fbx_basic_string<char> strName;

    FbxAMatrix kTransformMatrix;

    UintContainer bufMaterials;

    MeshAttribute bufMeshAttribute;
    fbx_vector<BoneCombination> bufBoneCombination;

    Vector3Container bufPositions;
    Uint3Container bufIndices;

    fbx_vector<LayerElement> bufLayers;

    SkinInfoContainer bufSkinData;
    BoneOffsetMatrixMap mapBoneDeformMatrices;
};

using FbxNodeToExportNode = fbx_unordered_map<fbxsdk::FbxNode*, FBXNode*, PointerHasher<fbxsdk::FbxNode*>>;
using ImportNodeToFbxNode = fbx_unordered_map<const FBXNode*, fbxsdk::FbxNode*, PointerHasher<const FBXNode*>>;

using PoseNodeList = fbx_unordered_set<fbxsdk::FbxNode*, PointerHasher<fbxsdk::FbxNode*>>;

// FBXShared_Animation ///////////////////////////////////////////////////////////////////////////////

template<typename T>
class AnimationKeyFrame{
public:
    AnimationKeyFrame(){}
    AnimationKeyFrame(const FbxTime& _time, const FBXAnimationInterpolationType& _type, const T& _local, const T& _world)
        :
        time(_time),
        type(_type),
        local(_local),
        world(_world)
    {}
    AnimationKeyFrame(const FbxTime& _time, const FBXAnimationInterpolationType& _type, const std::pair<T, T>& _value)
        :
        time(_time),
        type(_type),
        local(_value.first),
        world(_value.second)
    {}


public:
    FbxTime time;
    FBXAnimationInterpolationType type;
    T local;
    T world;
};
template<typename T>
using AnimationKeyFrames = fbx_vector<AnimationKeyFrame<T>>;

struct AnimationNode{
    fbxsdk::FbxNode* bindNode;

    AnimationKeyFrames<fbxsdk::FbxDouble3> scalingKeys;
    AnimationKeyFrames<fbxsdk::FbxDouble4> rotationKeys;
    AnimationKeyFrames<fbxsdk::FbxDouble3> translationKeys;
};
struct AnimationStack{
    FbxAnimStack* animStack;
    FbxTime endTime;
    fbx_vector<AnimationNode> nodes;
};

using AnimationNodes = fbx_vector<fbxsdk::FbxNode*>;

// FBXShared_Optimizer ///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////


// variables
// Common ////////////////////////////////////////////////////////////////////////////////////////////

extern FBXIOSetting shr_ioSetting;

extern FBXRoot* shr_root;

// FBXShared_Error ///////////////////////////////////////////////////////////////////////////////////

extern fbx_stack<fbx_string> shr_errorStack;
extern fbx_stack<fbx_string> shr_warningStack;

// FBXShared_Copy ///////////////////////////////////////////////////////////////////////////////////

// FBXShared_FbxSdk //////////////////////////////////////////////////////////////////////////////////

extern fbxsdk::FbxAxisSystem shr_axisSystem;
extern fbxsdk::FbxSystemUnit shr_systemUnit;

extern fbxsdk::FbxManager* shr_SDKManager;

extern fbxsdk::FbxScene* shr_scene;

// FBXShared_Converter ///////////////////////////////////////////////////////////////////////////////

// FBXShared_Bone ////////////////////////////////////////////////////////////////////////////////////

// FBXShared_Material ////////////////////////////////////////////////////////////////////////////////

extern MaterialTable shr_materialTable;

// FBXShared_Mesh ////////////////////////////////////////////////////////////////////////////////////

// FBXShared_Skin ////////////////////////////////////////////////////////////////////////////////////

// FBXShared_BoneCombination /////////////////////////////////////////////////////////////////////////

// FBXShared_Node ////////////////////////////////////////////////////////////////////////////////////

extern FbxNodeToExportNode shr_fbxNodeToExportNode;
extern ImportNodeToFbxNode shr_importNodeToFbxNode;
extern PoseNodeList shr_poseNodeList;

// FBXShared_Animation ///////////////////////////////////////////////////////////////////////////////

// FBXShared_Optimizer ///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////


// functions
// Common ////////////////////////////////////////////////////////////////////////////////////////////

extern void SHRCreateRoot();
extern void SHRDeleteRoot();

// FBXShared_Error ///////////////////////////////////////////////////////////////////////////////////

extern void SHRPushErrorMessage(const FBX_CHAR* strMessage, const FBX_CHAR* strCallPos);
extern void SHRPushErrorMessage(const fbx_string& strMessage, const FBX_CHAR* strCallPos);
extern void SHRPushErrorMessage(fbx_string&& strMessage, const FBX_CHAR* strCallPos);

extern void SHRPushWarningMessage(const FBX_CHAR* strMessage, const FBX_CHAR* strCallPos);
extern void SHRPushWarningMessage(const fbx_string& strMessage, const FBX_CHAR* strCallPos);
extern void SHRPushWarningMessage(fbx_string&& strMessage, const FBX_CHAR* strCallPos);

// FBXShared_Copy ///////////////////////////////////////////////////////////////////////////////////

extern void SHRRebindRoot(FBXRoot* dest, const FBXRoot* src);
extern void SHRRebindNode(FBXNode* dest, const FBXNode* src);
extern void SHRRebindAnimation(FBXAnimation& dest, const FBXAnimation& src);

// FBXShared_FbxSdk //////////////////////////////////////////////////////////////////////////////////

extern void SHRDestroyFbxSdkObjects();

// FBXShared_Converter ///////////////////////////////////////////////////////////////////////////////

extern bool SHRConvertNodes(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxNode* kNode);
extern bool SHRConvertAnimations(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene);
extern bool SHRPreparePointCaches(fbxsdk::FbxScene* kScene);
extern bool SHRConvertOjbects(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene);

// FBXShared_Bone ////////////////////////////////////////////////////////////////////////////////////

extern bool SHRLoadBoneNode(fbxsdk::FbxManager* kSDKManager, const fbxsdk::FbxNode* kNode, FBXBone* pNode);
extern bool SHRInitBoneNode(fbxsdk::FbxManager* kSDKManager, const FBXBone* pNode, fbxsdk::FbxNode* kNode);

// FBXShared_Material ////////////////////////////////////////////////////////////////////////////////

extern bool SHRLoadMaterials(const MaterialTable& materialTable, FBXDynamicArray<FBXMaterial>* pMaterials);
extern bool SHRStoreMaterials(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const FBXDynamicArray<FBXMaterial>& materialTable);

// FBXShared_Mesh ////////////////////////////////////////////////////////////////////////////////////

extern bool SHRLoadMeshFromNode(MaterialTable& materialTable, ControlPointRemap& controlPointRemap, fbxsdk::FbxNode* kNode, NodeData* pNodeData);

extern bool SHRInitMeshNode(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, ControlPointMergeMap& ctrlPointMergeMap, const FBXMesh* pNode, fbxsdk::FbxNode* kNode);

// FBXShared_Skin ////////////////////////////////////////////////////////////////////////////////////

extern fbxsdk::FbxAMatrix SHRGetBlendMatrix(const SkinData* skins, size_t count);

extern bool SHRLoadSkinFromNode(const ControlPointRemap& controlPointRemap, fbxsdk::FbxNode* kNode, NodeData* pNodeData);

extern bool SHRInitSkinData(fbxsdk::FbxManager* kSDKManager, PoseNodeList& poseNodeList, const ImportNodeToFbxNode& nodeBinder, const ControlPointMergeMap& ctrlPointMergeMap, const FBXSkinnedMesh* pNode, fbxsdk::FbxNode* kNode);

// FBXShared_BoneCombination /////////////////////////////////////////////////////////////////////////

extern void SHRGenerateMeshAttribute(NodeData* pNodeData);

// FBXShared_Node ////////////////////////////////////////////////////////////////////////////////////

extern bool SHRGenerateNodeTree(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, MaterialTable& materialTable, FbxNodeToExportNode& fbxNodeToExportNode, FBXNode** pRootNode);

extern fbxsdk::FbxNode* SHRStoreNode(fbxsdk::FbxManager* kSDKManager, ImportNodeToFbxNode& importNodeToFbxNode, fbxsdk::FbxNode* kParentNode, const FBXNode* pNode);
extern bool SHRStoreNodes(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, ImportNodeToFbxNode& importNodeToFbxNode, PoseNodeList& poseNodeList, const FBXNode* pRootNode);

extern bool SHRCreateBindPose(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const PoseNodeList& poseNodeList);

// FBXShared_Animation ///////////////////////////////////////////////////////////////////////////////

extern bool SHRLoadAnimation(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const AnimationNodes& kNodeTable);
extern bool SHRLoadAnimations(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const FbxNodeToExportNode& fbxNodeToExportNode, FBXDynamicArray<FBXAnimation>* pAnimations);

extern bool SHRStoreAnimation(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const ImportNodeToFbxNode& importNodeToFbxNode, const FBXAnimation* pAnimStack);
extern bool SHRStoreAnimations(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const ImportNodeToFbxNode& importNodeToFbxNode, const FBXDynamicArray<FBXAnimation>& animStacks);

// FBXShared_Optimizer ///////////////////////////////////////////////////////////////////////////////

extern void SHROptimizeMesh(NodeData* pNodeData);

//////////////////////////////////////////////////////////////////////////////////////////////////////
