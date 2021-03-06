/**
 * @file FBXMaterial.hpp
 * @date 2019/05/27
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#ifndef _FBXMATERIAL_HPP_
#define _FBXMATERIAL_HPP_


#include "FBXUtilites_independent.hpp"
#include "FBXType.hpp"

#include "FBXBase.hpp"


class FBXMaterial : public FBXBase{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_Material; }


public:
    FBXDynamicArray<FBX_CHAR> DiffuseTexturePath;

public:
    FBXDynamicArray<FBX_CHAR> Name;
};


#endif // _FBXMATERIAL_HPP_
