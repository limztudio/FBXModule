/**
* @file FBXBase.hpp
* @date 2019/10/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXType.hpp"


// abd0 ef00 0000 0000 0000 0000 c000 0000
// a: root identifier
// b: node identifier
// - d: bone identifier
// - e: mesh identifier
// - - f: skinned mesh identifier
// c: animation identifier


enum class FBXType : unsigned long{
    FBXType_Root = 1 << 31,

    FBXType_Node = 1 << 30,

    FBXType_Bone = FBXType_Node | (1 << 29),
    FBXType_Mesh = FBXType_Node | (1 << 27),
    FBXType_SkinnedMesh = FBXType_Mesh | (1 << 26),

    FBXType_Animation = 1 << 7,
};


class FBXBase{
public:
    virtual FBXType getID()const = 0;
};