/**
* @file FBXUtilites.hpp
* @date 2020/05/08
* @author Lim Taewoo (limztudio@gmail.com)
*/


#ifndef _FBXUTILITES_HPP_
#define _FBXUTILITES_HPP_


#include "FBXType.hpp"


template<typename T>
static inline bool FBXTypeHasMember(T target, T find){
    const auto t = (unsigned long)target;
    const auto f = (unsigned long)find;

    return (t & f) == f;
}

template<typename T>
static FBX_SIZE FBXGetMemoryLength(const T* p){
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

template<typename NODE, typename FUNC>
static void FBXIterateBackwardNode(NODE* p, FUNC func){
    if(p){
        func(p);
        FBXIterateBackwardNode(p->Parent, func);
    }
}
template<typename NODE, typename FUNC>
static void FBXBreakableIterateBackwardNode(NODE* p, FUNC func){
    if(p){
        if(!func(p))
            return;
        FBXBreakableIterateBackwardNode(p->Parent, func);
    }
}


#endif // _FBXUTILITES_HPP_
