/**
* @file FBXRoot.hpp
* @date 2020/05/08
* @author Lim Taewoo (limztudio@gmail.com)
*/


#ifndef _FBXROOT_HPP_
#define _FBXROOT_HPP_


#include "FBXBase.hpp"

#include "FBXNode.hpp"
#include "FBXMaterial.hpp"
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
    FBXDynamicArray<FBXAnimation> Animations;
    FBXDynamicArray<FBXMaterial> Materials;

public:
    FBXNode* Nodes;
};


#endif // _FBXROOT_HPP_
