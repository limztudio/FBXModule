/**
 * @file FBXMesh.hpp
 * @date 2019/10/11
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#ifndef _FBXMESH_HPP_
#define _FBXMESH_HPP_


#include "FBXType.hpp"

#include "FBXNode.hpp"


class FBXMeshAttribute{
public:
    unsigned long VertexStart;
    unsigned long VertexCount;

    unsigned long IndexStart;
    unsigned long IndexCount;
};

class FBXMeshLayerElement{
public:
    FBXDynamicArray<unsigned long> Material; // must have same count with Attributes; the index points 'Materials' from FBXMesh

public:
    FBXDynamicArray<FBXStaticArray<float, 4>> Color; // must have same count with Vertices; RGBA format

public:
    FBXDynamicArray<FBXStaticArray<float, 3>> Normal; // must have same count with Vertices
    FBXDynamicArray<FBXStaticArray<float, 3>> Binormal; // must have same count with Vertices
    FBXDynamicArray<FBXStaticArray<float, 3>> Tangent; // must have same count with Vertices

public:
    FBXDynamicArray<FBXStaticArray<float, 2>> Texcoord; // must have same count with Vertices
};


class FBXMesh : public FBXNode{
public:
    virtual FBXType getID()const{ return FBXType::FBXType_Mesh; }


public:
    FBXMesh(){}
    virtual ~FBXMesh(){}


public:
    FBXDynamicArray<unsigned long> Materials; // the index points 'Materials' from FBXRoot

public:
    FBXDynamicArray<FBXMeshAttribute> Attributes;

public:
    FBXDynamicArray<FBXStaticArray<unsigned long, 3>> Indices;
    FBXDynamicArray<FBXStaticArray<float, 3>> Vertices;

public:
    FBXDynamicArray<FBXMeshLayerElement> LayeredElements;
};


#endif // _FBXMESH_HPP_
