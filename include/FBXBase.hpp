/**
* @file FBXBase.hpp
* @date 2019/10/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#ifndef _FBXBASE_HPP_
#define _FBXBASE_HPP_


#include "FBXType.hpp"


// abef g000 0000 c000 d000 0000 0000 0000
// a: root identifier
// b: node identifier
// b e: bone identifier
// b f: mesh identifier
// b f g: skinned mesh identifier
// c: animation identifier
// d: material identifier


enum class FBXType : unsigned long{
    FBXType_Root = 1u << 31,

    FBXType_Node = 1u << 30,
    FBXType_Bone = FBXType_Node | (1u << 29),
    FBXType_Mesh = FBXType_Node | (1u << 28),
    FBXType_SkinnedMesh = FBXType_Mesh | (1u << 27),

    FBXType_Animation = 1u << 19,

    FBXType_Material = 1u << 15,
};


class FBXBase{
public:
    virtual FBXType getID()const = 0;


public:
    virtual ~FBXBase(){}
};


#endif // _FBXBASE_HPP_
