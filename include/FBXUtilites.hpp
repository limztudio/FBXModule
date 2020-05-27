/**
* @file FBXUtilites.hpp
* @date 2020/05/08
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXType.hpp"


template<typename T>
static FBX_SIZE FBXGetMemorylength(const T* p){
    FBX_SIZE i = 0;
    for(; (*p); ++p, ++i);
    return i;
}

template<typename NODE, typename FUNC>
static void FBXIterateNode(NODE* p, FUNC func){
    if(p){
        FBXIterateNode(p->Child, func);
        FBXIterateNode(p->Sibling, func);
        func(p);
    }
}
template<typename NODE, typename FUNC>
static void FBXBreakableIterateNode(NODE* p, FUNC func){
    if(p){
        FBXBreakableIterateNode(p->Child, func);
        FBXBreakableIterateNode(p->Sibling, func);
        if(!func(p))
            return;
    }
}
