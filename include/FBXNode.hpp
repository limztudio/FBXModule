/**
* @file FBXNode.hpp
* @date 2019/04/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXType.hpp"
#include "FBXBase.hpp"


class FBXNode : public FBXBase{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_Node; }


public:
    FBXNode()
        :
        Child(nullptr),
        Sibling(nullptr)
    {}
    virtual ~FBXNode(){}


public:
    FBXNode* Child;
    FBXNode* Sibling;

public:
    FBXStaticArray<float, 16> TransformMatrix;
};