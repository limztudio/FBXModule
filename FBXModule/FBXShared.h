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

using ControlPointMergeMap = eastl::vector<size_t>;

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

// FBXShared_BoneCombination /////////////////////////////////////////////////////////////////////////

struct MeshAttributeElement{
    unsigned long PolygonFirst;
    unsigned long PolygonLast;

    unsigned long VertexFirst;
    unsigned long VertexLast;
};

using MeshAttribute = eastl::vector<MeshAttributeElement>;
using BoneCombination = eastl::vector<fbxsdk::FbxCluster*>;

// FBXShared_Node ////////////////////////////////////////////////////////////////////////////////////

struct NodeData{
    eastl::string strName;

    FbxAMatrix kTransformMatrix;

    MeshAttribute bufMeshAttribute;
    eastl::vector<BoneCombination> bufBoneCombination;

    Vector3Container bufPositions;
    Int3Container bufIndices;

    eastl::vector<LayerElement> bufLayers;

    SkinInfoContainer bufSkinData;
    BoneOffsetMatrixMap mapBoneDeformMatrices;
};

using FbxNodeToExportNode = eastl::unordered_map<FbxNode*, FBXNode*>;
using ImportNodeToFbxNode = eastl::unordered_map<const FBXNode*, fbxsdk::FbxNode*>;

using PoseNodeList = eastl::unordered_set<FbxNode*>;

// FBXShared_Animation ///////////////////////////////////////////////////////////////////////////////

template<typename T>
class AnimationKeyFrame{
public:
    AnimationKeyFrame(){}
    AnimationKeyFrame(const fbxsdk::FbxAnimCurveKey& _curveKey, const T& _value)
        :
        curveKey(_curveKey),
        value(_value)
    {}
    AnimationKeyFrame(fbxsdk::FbxAnimCurveKey&& _curveKey, T&& _value)
        :
        curveKey(eastl::move(_curveKey)),
        value(eastl::move(_value))
    {}


public:
    fbxsdk::FbxAnimCurveKey curveKey;
    T value;
};
template<typename T>
using AnimationKeyFrames = eastl::vector<AnimationKeyFrame<T>>;

class AnimationNode{
public:
    inline bool isEmpty()const{
        if(!scalingKeys.empty())
            return false;
        if(!rotationKeys.empty())
            return false;
        if(!translationKeys.empty())
            return false;

        return true;
    }


public:
    fbxsdk::FbxNode* bindNode;

    AnimationKeyFrames<fbxsdk::FbxDouble3> scalingKeys;
    AnimationKeyFrames<fbxsdk::FbxDouble4> rotationKeys;
    AnimationKeyFrames<fbxsdk::FbxDouble3> translationKeys;
};
struct AnimationLayer{
    eastl::string strName;
    eastl::vector<AnimationNode> nodes;
};
struct AnimationStack{
    eastl::string strName;
    eastl::vector<AnimationLayer> layers;
};

using AnimationNodes = eastl::vector<fbxsdk::FbxNode*>;

// FBXShared_Optimizer ///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////


// variables
// Common ////////////////////////////////////////////////////////////////////////////////////////////

extern FBXIOSetting shr_ioSetting;

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

extern bool SHRInitMeshNode(fbxsdk::FbxManager* kSDKManager, ControlPointMergeMap& ctrlPointMergeMap, const FBXMesh* pNode, fbxsdk::FbxNode* kNode);

// FBXShared_Skin ////////////////////////////////////////////////////////////////////////////////////

extern fbxsdk::FbxAMatrix SHRGetBlendMatrix(const SkinData* skins, size_t count);

extern bool SHRLoadSkinFromNode(const ControlPointRemap& controlPointRemap, fbxsdk::FbxNode* kNode, NodeData* pNodeData);

extern bool SHRInitSkinData(fbxsdk::FbxManager* kSDKManager, PoseNodeList& poseNodeList, const ImportNodeToFbxNode& nodeBinder, const ControlPointMergeMap& ctrlPointMergeMap, const FBXSkinnedMesh* pNode, fbxsdk::FbxNode* kNode);

// FBXShared_BoneCombination /////////////////////////////////////////////////////////////////////////

extern void SHRGenerateMeshAttribute(NodeData* pNodeData);

// FBXShared_Node ////////////////////////////////////////////////////////////////////////////////////

extern bool SHRGenerateNodeTree(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, FbxNodeToExportNode& fbxNodeToExportNode);

extern bool SHRStoreNodes(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, ImportNodeToFbxNode& importNodeToFbxNode, PoseNodeList& poseNodeList, const FBXNode* pRootNode);
extern fbxsdk::FbxNode* SHRStoreNode(fbxsdk::FbxManager* kSDKManager, ImportNodeToFbxNode& importNodeToFbxNode, fbxsdk::FbxNode* kParentNode, const FBXNode* pNode);

extern bool SHRCreateBindPose(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const PoseNodeList& poseNodeList);

// FBXShared_Animation ///////////////////////////////////////////////////////////////////////////////

extern bool SHRLoadAnimation(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const AnimationNodes& kNodeTable);
extern bool SHRLoadAnimations(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const FbxNodeToExportNode& fbxNodeToExportNode);

extern bool SHRStoreAnimation(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const ImportNodeToFbxNode& importNodeToFbxNode, const FBXAnimation* pAnimStack);
extern bool SHRStoreAnimations(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const ImportNodeToFbxNode& importNodeToFbxNode, const FBXAnimation* pRootAnimStack);

// FBXShared_Optimizer ///////////////////////////////////////////////////////////////////////////////

extern void SHROptimizeMesh(NodeData* pNodeData);

//////////////////////////////////////////////////////////////////////////////////////////////////////
