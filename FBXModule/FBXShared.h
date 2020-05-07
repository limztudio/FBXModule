/**
* @file FBXUtilites.h
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include <eastl/stack.h>
#include <eastl/vector.h>
#include <eastl/unordered_set.h>
#include <eastl/unordered_map.h>
#include <eastl/string.h>

#include <fbxsdk.h>

#include "FBXUtilites.h"

#include <FBXType.hpp>
#include <FBXBase.hpp>
#include <FBXNode.hpp>
#include <FBXBone.hpp>
#include <FBXMesh.hpp>
#include <FBXSkinnedMesh.hpp>
#include <FBXAnimation.hpp>


// structures
// FBXShared_Error ///////////////////////////////////////////////////////////////////////////////////

// FBXShared_FbxSdk //////////////////////////////////////////////////////////////////////////////////

// FBXShared_Mesh ////////////////////////////////////////////////////////////////////////////////////

struct TexcoordTable{
    eastl::string name;
    eastl::vector<fbxsdk::FbxDouble2> table;
};

using IntContainer = eastl::vector<int>;
using Int3Container = eastl::vector<Int3>;
using Vector3Container = eastl::vector<fbxsdk::FbxDouble3>;
using Vector4Container = eastl::vector<fbxsdk::FbxDouble4>;
using Unit3Container = eastl::vector<fbxsdk::FbxDouble3>;

using ControlPointRemap = eastl::vector<eastl::unordered_set<int>>;

struct LayerElement{
    Int3Container smoothings;
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
    Int3Container bufEdges;

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

//////////////////////////////////////////////////////////////////////////////////////////////////////


// variables
// FBXShared_Error ///////////////////////////////////////////////////////////////////////////////////

extern eastl::stack<eastl::string> shr_errorStack;

// FBXShared_FbxSdk //////////////////////////////////////////////////////////////////////////////////

extern fbxsdk::FbxAxisSystem shr_axisSystem;
extern fbxsdk::FbxSystemUnit shr_systemUnit;

extern fbxsdk::FbxManager* shr_SDKManager;

extern fbxsdk::FbxScene* shr_scene;

// FBXShared_Mesh ////////////////////////////////////////////////////////////////////////////////////

// FBXShared_Skin ////////////////////////////////////////////////////////////////////////////////////

// FBXShared_Node ////////////////////////////////////////////////////////////////////////////////////

extern FBXNode* shr_rootNode;

extern eastl::unordered_map<fbxsdk::FbxNode*, FBXNode*> shr_fbxNodeToExportNode;

// FBXShared_Animation ///////////////////////////////////////////////////////////////////////////////

extern FBXAnimation* shr_firstAnimation;

//////////////////////////////////////////////////////////////////////////////////////////////////////


// functions
// FBXShared_Error ///////////////////////////////////////////////////////////////////////////////////

extern void SHRPushErrorMessage(const char* strMessage, const char* strCallPos);

// FBXShared_FbxSdk //////////////////////////////////////////////////////////////////////////////////

extern void SHRDestroyFbxSdkObjects();

// FBXShared_Mesh ////////////////////////////////////////////////////////////////////////////////////

extern bool SHRLoadMeshFromNode(ControlPointRemap& controlPointRemap, fbxsdk::FbxNode* kNode, NodeData* pNodeData);

// FBXShared_Skin ////////////////////////////////////////////////////////////////////////////////////

extern fbxsdk::FbxAMatrix SHRGetBlendMatrix(const SkinData* skins, size_t count);

extern bool SHRLoadSkinFromNode(ControlPointRemap& controlPointRemap, fbxsdk::FbxNode* kNode, NodeData* pNodeData);

// FBXShared_Node ////////////////////////////////////////////////////////////////////////////////////

extern void SHRDeleteAllNodes();
extern void SHRDeleteNode(FBXNode* node);

extern bool SHRGenerateNodeTree(FbxManager* kSDKManager, fbxsdk::FbxScene* kScene);

// FBXShared_Animation ///////////////////////////////////////////////////////////////////////////////

extern void SHRDeleteAllAnimations();
extern void SHRDeleteAnimation(FBXAnimation* animation);

extern bool SHRLoadAnimation(FbxManager* kSDKManager, fbxsdk::FbxScene* kScene);

//////////////////////////////////////////////////////////////////////////////////////////////////////