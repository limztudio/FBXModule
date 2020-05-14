/**
* @file FBXUtilites.h
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include <eastl/stack.h>
#include <eastl/vector.h>
#include <eastl/fixed_vector.h>
#include <eastl/unordered_set.h>
#include <eastl/unordered_map.h>
#include <eastl/string.h>

#include <fbxsdk.h>

#include "FBXUtilites.h"

#include <FBXModule.hpp>


// structures
// Common ////////////////////////////////////////////////////////////////////////////////////////////

using IntContainer = eastl::vector<int>;
using Int3Container = eastl::vector<Int3>;
using Vector3Container = eastl::vector<fbxsdk::FbxDouble3>;
using Vector4Container = eastl::vector<fbxsdk::FbxDouble4>;
using Unit3Container = eastl::vector<fbxsdk::FbxDouble3>;

// FBXShared_Error ///////////////////////////////////////////////////////////////////////////////////

// FBXShared_FbxSdk //////////////////////////////////////////////////////////////////////////////////

// FBXShared_Bone ////////////////////////////////////////////////////////////////////////////////////

// FBXShared_Mesh ////////////////////////////////////////////////////////////////////////////////////

struct TexcoordTable{
    eastl::string name;
    eastl::vector<fbxsdk::FbxDouble2> table;
};

using ControlPointRemap = eastl::vector<eastl::unordered_set<int>>;

struct LayerElement{
    IntContainer materials;

    Vector4Container colors;
    Unit3Container normals;
    Unit3Container binormals;
    Unit3Container tangents;
    TexcoordTable texcoords;
};

// FBXShared_Skin ////////////////////////////////////////////////////////////////////////////////////

struct SkinInfo{
    fbxsdk::FbxCluster* cluster;
    fbxsdk::FbxDouble weight;
};
struct SkinData{
    fbxsdk::FbxAMatrix matrix;
    fbxsdk::FbxDouble weight;
};

using SkinInfoContainer = eastl::vector<eastl::vector<SkinInfo>>;

using BoneOffsetMatrixMap = eastl::unordered_map<fbxsdk::FbxCluster*, fbxsdk::FbxAMatrix>;

// FBXShared_Node ////////////////////////////////////////////////////////////////////////////////////

struct NodeData{
    eastl::string strName;

    FbxAMatrix kTransformMatrix;

    Vector3Container bufPositions;
    Int3Container bufIndices;

    eastl::vector<LayerElement> bufLayers;

    SkinInfoContainer bufSkinData;
    BoneOffsetMatrixMap mapBoneDeformMatrices;
};

// FBXShared_Animation ///////////////////////////////////////////////////////////////////////////////

struct AnimationElement{
    fbxsdk::FbxDouble time;


};

using AnimationQueue = eastl::vector<AnimationElement>;
using AnimationList = eastl::unordered_map<fbxsdk::FbxNode*, AnimationQueue>;

struct AnimationData{
    eastl::string strName;

    AnimationList animationList;
};

// FBXShared_Optimizer ///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////


// variables
// Common ////////////////////////////////////////////////////////////////////////////////////////////

extern FBXRoot* shr_root;

// FBXShared_Error ///////////////////////////////////////////////////////////////////////////////////

extern eastl::stack<eastl::string> shr_errorStack;

// FBXShared_FbxSdk //////////////////////////////////////////////////////////////////////////////////

extern fbxsdk::FbxAxisSystem shr_axisSystem;
extern fbxsdk::FbxSystemUnit shr_systemUnit;

extern fbxsdk::FbxManager* shr_SDKManager;

extern fbxsdk::FbxScene* shr_scene;

// FBXShared_Bone ////////////////////////////////////////////////////////////////////////////////////

// FBXShared_Mesh ////////////////////////////////////////////////////////////////////////////////////

// FBXShared_Skin ////////////////////////////////////////////////////////////////////////////////////

// FBXShared_Node ////////////////////////////////////////////////////////////////////////////////////

extern eastl::unordered_map<fbxsdk::FbxNode*, FBXNode*> shr_fbxNodeToExportNode;
extern eastl::unordered_map<const FBXNode*, fbxsdk::FbxNode*> shr_ImportedNodeToFbxNode;

// FBXShared_Animation ///////////////////////////////////////////////////////////////////////////////

// FBXShared_Optimizer ///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////


// functions
// Common ////////////////////////////////////////////////////////////////////////////////////////////

extern void SHRCreateRoot();
extern void SHRDeleteRoot();

extern void SHRNodeBinder(FBXNode* dest, const FBXNode* src);

extern void SHRCopyRoot(FBXRoot* dest, const FBXRoot* src);
extern void SHRCopyNode(FBXNode* dest, const FBXNode* src);
extern void SHRCopyAnimation(FBXAnimation* dest, const FBXAnimation* src);

// FBXShared_Error ///////////////////////////////////////////////////////////////////////////////////

extern void SHRPushErrorMessage(const char* strMessage, const char* strCallPos);
extern void SHRPushErrorMessage(const eastl::string& strMessage, const char* strCallPos);
extern void SHRPushErrorMessage(eastl::string&& strMessage, const char* strCallPos);

// FBXShared_FbxSdk //////////////////////////////////////////////////////////////////////////////////

extern void SHRDestroyFbxSdkObjects();

// FBXShared_Bone ////////////////////////////////////////////////////////////////////////////////////

extern bool SHRInitBoneNode(fbxsdk::FbxManager* kSDKManager, const FBXBone* pNode, fbxsdk::FbxNode* kNode);

// FBXShared_Mesh ////////////////////////////////////////////////////////////////////////////////////

extern bool SHRLoadMeshFromNode(ControlPointRemap& controlPointRemap, fbxsdk::FbxNode* kNode, NodeData* pNodeData);

extern bool SHRInitMeshNode(fbxsdk::FbxManager* kSDKManager, const FBXMesh* pNode, fbxsdk::FbxNode* kNode);
extern bool SHRInitSkinnedMeshNode(fbxsdk::FbxManager* kSDKManager, const FBXSkinnedMesh* pNode, fbxsdk::FbxNode* kNode);

// FBXShared_Skin ////////////////////////////////////////////////////////////////////////////////////

extern fbxsdk::FbxAMatrix SHRGetBlendMatrix(const SkinData* skins, size_t count);

extern bool SHRLoadSkinFromNode(const ControlPointRemap& controlPointRemap, fbxsdk::FbxNode* kNode, NodeData* pNodeData);

// FBXShared_Node ////////////////////////////////////////////////////////////////////////////////////

extern bool SHRGenerateNodeTree(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene);

extern bool SHRStoreNodes(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const FBXNode* pRootNode);
extern fbxsdk::FbxNode* SHRStoreNode(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxNode* kParentNode, const FBXNode* pNode);

// FBXShared_Animation ///////////////////////////////////////////////////////////////////////////////

extern bool SHRLoadAnimation(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene);

// FBXShared_Optimizer ///////////////////////////////////////////////////////////////////////////////

extern void SHROptimizeMesh(ControlPointRemap& controlPointRemap, NodeData* pNodeData);

//////////////////////////////////////////////////////////////////////////////////////////////////////
