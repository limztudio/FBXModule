/**
* @file FBXAnimation.hpp
* @date 2019/04/19
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXBase.hpp"


class FBXAnimation : public FBXBase{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_Animation; }


public:
    FBXAnimation()
        :
        Next(nullptr)
    {}
    virtual ~FBXAnimation(){}


public:
    FBXAnimation* Next;
};