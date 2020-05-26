/**
* @file FBXBone.hpp
* @date 2019/10/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXNode.hpp"


class FBXBone : public FBXNode{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_Bone; }


public:
    FBXBone()
        :
        Size(100.f),
        Length(1.f)
    {}
    virtual ~FBXBone(){}


public:
    float Size;
    float Length;
};
