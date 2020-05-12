/**
* @file FBXMesh.hpp
* @date 2019/10/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXType.hpp"

#include "FBXNode.hpp"


class FBXMeshLayerElement{
public:
    FBXDynamicArray<FBXStaticArray<long, 3>> Smoothing;
    FBXDynamicArray<long> Material;

public:
    FBXDynamicArray<FBXStaticArray<float, 4>> Color;

public:
    FBXDynamicArray<FBXStaticArray<float, 3>> Normal;
    FBXDynamicArray<FBXStaticArray<float, 3>> Binormal;
    FBXDynamicArray<FBXStaticArray<float, 3>> Tangent;

public:
    FBXDynamicArray<FBXStaticArray<float, 2>> Texcoord;
};


class FBXMesh : public FBXNode{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_Mesh; }


public:
    FBXMesh(){}
    virtual ~FBXMesh(){}


public:
    FBXDynamicArray<FBXStaticArray<long, 3>> Indices;
    FBXDynamicArray<FBXStaticArray<long, 3>> Edges;
    FBXDynamicArray<FBXStaticArray<float, 3>> Vertices;

public:
    FBXDynamicArray<FBXMeshLayerElement> LayeredVertices;
};
