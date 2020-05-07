/**
* @file FBXBase.hpp
* @date 2019/10/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXType.hpp"


enum class FBXType : unsigned char{
    FBXType_Node,

    FBXType_Bone,
    FBXType_Mesh,
    FBXType_SkinnedMesh,

    FBXType_Animation,
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
            free(Name);
    }


public:
    char* Name;
};