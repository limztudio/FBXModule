/**
* @file FBXUtilites.h
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include <stack>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>

#include <fbxsdk.h>

#include "FBXUtilites.h"

#include <FBXModule.hpp>


// structures
// Common ////////////////////////////////////////////////////////////////////////////////////////////

using IntContainer = std::vector<int>;
using Int3Container = std::vector<Int3>;
using Vector3Container = std::vector<fbxsdk::FbxDouble3>;
using Vector4Container = std::vector<fbxsdk::FbxDouble4>;
using Unit3Container = std::vector<fbxsdk::FbxDouble3>;

// FBXShared_Error ///////////////////////////////////////////////////////////////////////////////////

// FBXShared_FbxSdk //////////////////////////////////////////////////////////////////////////////////

// FBXShared_Converter ///////////////////////////////////////////////////////////////////////////////

// FBXShared_Bone ////////////////////////////////////////////////////////////////////////////////////

// FBXShared_Material ////////////////////////////////////////////////////////////////////////////////

struct MaterialElement{
    std::string name;

    std::string diffusePath;
};

// FBXShared_Mesh ////////////////////////////////////////////////////////////////////////////////////

struct TexcoordTable{
    std::string name;
    std::vector<fbxsdk::FbxDouble2> table;
};

using ControlPointRemap = std::vector<std::unordered_set<int>>;

struct LayerElement{
    IntContainer materials;

    Vector4Container colors;
    Unit3Container normals;
    Unit3Container binormals;
    Unit3Container tangents;
    TexcoordTable texcoords;
};

using ControlPointMergeMap = std::vector<size_t>;

// FBXShared_Skin ////////////////////////////////////////////////////////////////////////////////////

struct SkinInfo{
    fbxsdk::FbxCluster* cluster;
    fbxsdk::FbxDouble weight;
};
struct SkinData{
    fbxsdk::FbxAMatrix matrix;
    fbxsdk::FbxDouble weight;
};

using SkinInfoContainer = std::vector<std::vector<SkinInfo>>;

using BoneOffsetMatrixMap = std::unordered_map<fbxsdk::FbxCluster*, fbxsdk::FbxAMatrix, PointerHasher<fbxsdk::FbxCluster*>>;

// FBXShared_BoneCombination /////////////////////////////////////////////////////////////////////////

struct MeshAttributeElement{
    unsigned long PolygonFirst;
    unsigned long PolygonLast;

    unsigned long VertexFirst;
    unsigned long VertexLast;
};

using MeshAttribute = std::vector<MeshAttributeElement>;
using BoneCombination = std::vector<fbxsdk::FbxCluster*>;

// FBXShared_Node ////////////////////////////////////////////////////////////////////////////////////

struct NodeData{
    std::string strName;

    FbxAMatrix kTransformMatrix;

    std::vector<MaterialElement> bufMaterials;

    MeshAttribute bufMeshAttribute;
    std::vector<BoneCombination> bufBoneCombination;

    Vector3Container bufPositions;
    Int3Container bufIndices;

    std::vector<LayerElement> bufLayers;

    SkinInfoContainer bufSkinData;
    BoneOffsetMatrixMap mapBoneDeformMatrices;
};

using FbxNodeToExportNode = std::unordered_map<fbxsdk::FbxNode*, FBXNode*, PointerHasher<fbxsdk::FbxNode*>>;
using ImportNodeToFbxNode = std::unordered_map<const FBXNode*, fbxsdk::FbxNode*, PointerHasher<const FBXNode*>>;

using PoseNodeList = std::unordered_set<fbxsdk::FbxNode*, PointerHasher<fbxsdk::FbxNode*>>;

// FBXShared_Animation ///////////////////////////////////////////////////////////////////////////////

template<typename T>
class AnimationKeyFrame{
public:
    AnimationKeyFrame(){}
    AnimationKeyFrame(const FbxTime& _time, const FBXAnimationInterpolationType& _type, const T& _value)
        :
        time(_time),
        type(_type),
        value(_value)
    {}


public:
    FbxTime time;
    FBXAnimationInterpolationType type;
    T value;
};
template<typename T>
using AnimationKeyFrames = std::vector<AnimationKeyFrame<T>>;

struct AnimationNode{
    fbxsdk::FbxNode* bindNode;

    AnimationKeyFrames<fbxsdk::FbxDouble3> scalingKeys;
    AnimationKeyFrames<fbxsdk::FbxDouble4> rotationKeys;
    AnimationKeyFrames<fbxsdk::FbxDouble3> translationKeys;
};
struct AnimationStack{
    FbxAnimStack* animStack;
    FbxTime endTime;
    std::vector<AnimationNode> nodes;
};

using AnimationNodes = std::vector<fbxsdk::FbxNode*>;

// FBXShared_Optimizer ///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////


// variables
// Common ////////////////////////////////////////////////////////////////////////////////////////////

extern FBXIOSetting shr_ioSetting;

extern FBXRoot* shr_root;

// FBXShared_Error ///////////////////////////////////////////////////////////////////////////////////

extern std::stack<std::string> shr_errorStack;

// FBXShared_FbxSdk //////////////////////////////////////////////////////////////////////////////////

extern fbxsdk::FbxAxisSystem shr_axisSystem;
extern fbxsdk::FbxSystemUnit shr_systemUnit;

extern fbxsdk::FbxManager* shr_SDKManager;

extern fbxsdk::FbxScene* shr_scene;

// FBXShared_Converter ///////////////////////////////////////////////////////////////////////////////

// FBXShared_Bone ////////////////////////////////////////////////////////////////////////////////////

// FBXShared_Material ////////////////////////////////////////////////////////////////////////////////

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
extern void SHRPushErrorMessage(const std::string& strMessage, const char* strCallPos);
extern void SHRPushErrorMessage(std::string&& strMessage, const char* strCallPos);

// FBXShared_FbxSdk //////////////////////////////////////////////////////////////////////////////////

extern void SHRDestroyFbxSdkObjects();

// FBXShared_Converter ///////////////////////////////////////////////////////////////////////////////

extern bool SHRConvertNodes(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxNode* kNode);
extern bool SHRConvertAnimations(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene);
extern bool SHRConvertOjbects(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene);

// FBXShared_Bone ////////////////////////////////////////////////////////////////////////////////////

extern bool SHRInitBoneNode(fbxsdk::FbxManager* kSDKManager, const FBXBone* pNode, fbxsdk::FbxNode* kNode);

// FBXShared_Material ////////////////////////////////////////////////////////////////////////////////

extern bool SHRLoadMaterial(MaterialElement& iMaterial, fbxsdk::FbxSurfaceMaterial* kMaterial);
extern fbxsdk::FbxSurfaceMaterial* SHRCreateMaterial(FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const FBXMeshMaterial* pMaterial);

// FBXShared_Mesh ////////////////////////////////////////////////////////////////////////////////////

extern bool SHRLoadMeshFromNode(ControlPointRemap& controlPointRemap, fbxsdk::FbxNode* kNode, NodeData* pNodeData);

extern bool SHRInitMeshNode(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, ControlPointMergeMap& ctrlPointMergeMap, const FBXMesh* pNode, fbxsdk::FbxNode* kNode);

// FBXShared_Skin ////////////////////////////////////////////////////////////////////////////////////

extern fbxsdk::FbxAMatrix SHRGetBlendMatrix(const SkinData* skins, size_t count);

extern bool SHRLoadSkinFromNode(const ControlPointRemap& controlPointRemap, fbxsdk::FbxNode* kNode, NodeData* pNodeData);

extern bool SHRInitSkinData(fbxsdk::FbxManager* kSDKManager, PoseNodeList& poseNodeList, const ImportNodeToFbxNode& nodeBinder, const ControlPointMergeMap& ctrlPointMergeMap, const FBXSkinnedMesh* pNode, fbxsdk::FbxNode* kNode);

// FBXShared_BoneCombination /////////////////////////////////////////////////////////////////////////

extern void SHRGenerateMeshAttribute(NodeData* pNodeData);

// FBXShared_Node ////////////////////////////////////////////////////////////////////////////////////

extern bool SHRGenerateNodeTree(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, FbxNodeToExportNode& fbxNodeToExportNode);

extern fbxsdk::FbxNode* SHRStoreNode(fbxsdk::FbxManager* kSDKManager, ImportNodeToFbxNode& importNodeToFbxNode, fbxsdk::FbxNode* kParentNode, const FBXNode* pNode);
extern bool SHRStoreNodes(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, ImportNodeToFbxNode& importNodeToFbxNode, PoseNodeList& poseNodeList, const FBXNode* pRootNode);

extern bool SHRCreateBindPose(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const PoseNodeList& poseNodeList);

// FBXShared_Animation ///////////////////////////////////////////////////////////////////////////////

extern bool SHRLoadAnimation(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const AnimationNodes& kNodeTable);
extern bool SHRLoadAnimations(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const FbxNodeToExportNode& fbxNodeToExportNode);

extern bool SHRStoreAnimation(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const ImportNodeToFbxNode& importNodeToFbxNode, const FBXAnimation* pAnimStack);
extern bool SHRStoreAnimations(fbxsdk::FbxManager* kSDKManager, fbxsdk::FbxScene* kScene, const ImportNodeToFbxNode& importNodeToFbxNode, const FBXAnimation* pRootAnimStack);

// FBXShared_Optimizer ///////////////////////////////////////////////////////////////////////////////

extern void SHROptimizeMesh(NodeData* pNodeData);

//////////////////////////////////////////////////////////////////////////////////////////////////////
