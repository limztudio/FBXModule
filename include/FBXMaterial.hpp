/**
* @file FBXMaterial.hpp
* @date 2019/05/27
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXUtilites.hpp"
#include "FBXType.hpp"

#include "FBXBase.hpp"


class FBXMaterial : public FBXBase{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_Material; }


public:
    FBXDynamicArray<char> DiffuseTexturePath;
};
