/**
* @file FBXAnimation.hpp
* @date 2019/04/19
* @author Lim Taewoo (limztudio@gmail.com)
*/


#ifndef _FBXANIMATION_HPP_
#define _FBXANIMATION_HPP_


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
    T Value;

public:
    float Time;
    FBXAnimationInterpolationType InterpolationType;
};
class FBXAnimationNode{
public:
    FBXDynamicArray<FBXAnimationKeyFrame<FBXStaticArray<float, 3>>> ScalingKeys;
    FBXDynamicArray<FBXAnimationKeyFrame<FBXStaticArray<float, 4>>> RotationKeys;
    FBXDynamicArray<FBXAnimationKeyFrame<FBXStaticArray<float, 3>>> TranslationKeys;

public:
    FBXNode* BindNode;
};


class FBXAnimation : public FBXBase{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_Animation; }


public:
    FBXDynamicArray<FBXAnimationNode> AnimationNodes;
    float EndTime;

public:
    FBXDynamicArray<TCHAR> Name;
};


#endif // _FBXANIMATION_HPP_
