/**
* @file FBXRoot.hpp
* @date 2020/05/08
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXBase.hpp"

#include "FBXNode.hpp"
#include "FBXAnimation.hpp"


class FBXRoot : public FBXBase{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_Root; }


public:
    FBXRoot()
        :
        Nodes(nullptr)
    {}
    virtual ~FBXRoot(){
        if(Nodes)
            FBXDelete(Nodes);
    }


public:
    FBXNode* Nodes;

public:
    FBXDynamicArray<FBXAnimation> Animations;
};
