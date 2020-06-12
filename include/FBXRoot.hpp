/**
* @file FBXRoot.hpp
* @date 2020/05/08
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXBase.hpp"

#include "FBXNode.hpp"
#include "FBXMaterial.hpp"
#include "FBXAnimation.hpp"


class FBXRoot : public FBXBase{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_Root; }
    virtual const char* getName()const{ return nullptr; }


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
    FBXDynamicArray<FBXAnimation> Animations;
    FBXDynamicArray<FBXMaterial> Materials;

public:
    FBXNode* Nodes;
};
