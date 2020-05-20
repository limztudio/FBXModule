/**
* @file FBXSkinnedMesh.hpp
* @date 2019/10/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXType.hpp"

#include "FBXMesh.hpp"


typedef FBXDynamicArray<FBXDynamicArray<FBXNode*>> FBXBoneCombination;
class FBXSkinElement{
public:
    FBXNode* BindNode;
    float Weight;
};
class FBXSkinDeformElement{
public:
    FBXNode* TargetNode;

public:
    FBXStaticArray<float, 16> DeformMatrix;
};


class FBXSkinnedMesh : public FBXMesh{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_SkinnedMesh; }


public:
    FBXSkinnedMesh(){}
    virtual ~FBXSkinnedMesh(){}


public:
    FBXBoneCombination BoneCombinations;
    FBXDynamicArray<FBXDynamicArray<FBXSkinElement>> SkinInfos;
    FBXDynamicArray<FBXSkinDeformElement> SkinDeforms;
};
