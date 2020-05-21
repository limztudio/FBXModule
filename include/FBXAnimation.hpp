/**
* @file FBXAnimation.hpp
* @date 2019/04/19
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXUtilites.hpp"

#include "FBXBase.hpp"


class FBXNode;


enum class FBXAnimationInterpolationType : unsigned char{
    FBXAnimationInterpolationType_Stepped,
    FBXAnimationInterpolationType_Linear,
};

template<typename T>
class FBXAnimationKeyFrame{
public:
    float Time;
    FBXAnimationInterpolationType InterpolationType;

public:
    T Value;
};
class FBXAnimationNode{
public:
    FBXNode* BindNode;

public:
    FBXDynamicArray<FBXAnimationKeyFrame<FBXStaticArray<float, 3>>> ScalingKeys;
    FBXDynamicArray<FBXAnimationKeyFrame<FBXStaticArray<float, 4>>> RotationKeys;
    FBXDynamicArray<FBXAnimationKeyFrame<FBXStaticArray<float, 3>>> TranslationKeys;
};
class FBXAnimationLayer{
public:
    FBXDynamicArray<char> Name;

public:
    FBXDynamicArray<FBXAnimationNode> AnimationNodes;
};


class FBXAnimation : public FBXBase{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_Animation; }


public:
    FBXAnimation()
        :
        Next(nullptr)
    {}
    virtual ~FBXAnimation(){
        if(Next)
            FBXDelete(Next);
    }


public:
    FBXAnimation* Next;

public:
    FBXDynamicArray<FBXAnimationLayer> AnimationLayers;
};
