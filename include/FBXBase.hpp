/**
* @file FBXBase.hpp
* @date 2019/10/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXType.hpp"


// abde f000 0000 0000 0000 0000 c000 0000
// a: root identifier
// b: node identifier
// b d: bone identifier
// b e: mesh identifier
// b e f: skinned mesh identifier
// c: animation identifier


enum class FBXType : unsigned long{
    FBXType_Root = 1u << 31,

    FBXType_Node = 1u << 30,

    FBXType_Bone = FBXType_Node | (1u << 29),
    FBXType_Mesh = FBXType_Node | (1u << 28),
    FBXType_SkinnedMesh = FBXType_Mesh | (1u << 27),

    FBXType_Animation = 1u << 7,
};


class FBXBase{
public:
    virtual FBXType getID()const = 0;


public:
    FBXBase()
        :
        Name(nullptr)
    {}
    virtual ~FBXBase(){
        if(Name)
            FBXFree(Name);
    }


public:
    char* Name;
};
