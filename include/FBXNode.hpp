/**
* @file FBXNode.hpp
* @date 2019/04/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#ifndef _FBXNODE_HPP_
#define _FBXNODE_HPP_


#include "FBXUtilites.hpp"
#include "FBXType.hpp"

#include "FBXBase.hpp"


class FBXNode : public FBXBase{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_Node; }


public:
    FBXNode()
        :
        TransformMatrix({ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }),

        Parent(nullptr),
        Child(nullptr),
        Sibling(nullptr)
    {}
    virtual ~FBXNode(){
        if(Child)
            FBXDelete(Child);
        if(Sibling)
            FBXDelete(Sibling);
    }


public:
    FBXStaticArray<float, 16> TransformMatrix;

public:
    FBXNode* Parent;
    FBXNode* Child;
    FBXNode* Sibling;

public:
    FBXDynamicArray<char> Name;
};


#endif // _FBXNODE_HPP_
