/**
* @file FBXSkinnedMesh.hpp
* @date 2019/10/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#ifndef _FBXSKINNEDMESH_HPP_
#define _FBXSKINNEDMESH_HPP_


#include "FBXType.hpp"

#include "FBXMesh.hpp"


class FBXSkinElement{
public:
    FBXNode* BindNode;
    float Weight;
};
class FBXSkinDeformElement{
public:
    FBXStaticArray<float, 16> TransformMatrix;
    FBXStaticArray<float, 16> LinkMatrix;

public:
    FBXNode* TargetNode;
};


class FBXSkinnedMesh : public FBXMesh{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_SkinnedMesh; }


public:
    FBXSkinnedMesh(){}
    virtual ~FBXSkinnedMesh(){}


public:
    FBXDynamicArray<FBXDynamicArray<FBXNode*>> BoneCombinations; // must have same count with Attributes
    FBXDynamicArray<FBXDynamicArray<FBXSkinElement>> SkinInfos; // must have same count with Vertices
    FBXDynamicArray<FBXSkinDeformElement> SkinDeforms;
};


#endif // _FBXSKINNEDMESH_HPP_
