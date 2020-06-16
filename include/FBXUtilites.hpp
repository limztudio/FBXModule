/**
* @file FBXUtilites.hpp
* @date 2020/05/08
* @author Lim Taewoo (limztudio@gmail.com)
*/


#ifndef _FBXUTILITES_HPP_
#define _FBXUTILITES_HPP_


#include "FBXType.hpp"


namespace __hidden_FBXModule{
    struct _IteratorThrower{};

    template<typename NODE, typename FUNC>
    static void _breakableIterateNode(NODE* pNode, FUNC func){
        if(pNode){
            FBXBreakableIterateNode(pNode->Child, func);
            FBXBreakableIterateNode(pNode->Sibling, func);
            if(!func(pNode))
                throw _IteratorThrower();
        }
    }
};


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
static void FBXIterateNode(NODE* pNode, FUNC func){
    if(pNode){
        FBXIterateNode(pNode->Child, func);
        FBXIterateNode(pNode->Sibling, func);
        func(pNode);
    }
}
template<typename NODE, typename FUNC>
static void FBXBreakableIterateNode(NODE* pNode, FUNC func){
    try{
        __hidden_FBXModule::_breakableIterateNode(pNode, func);
    }
    catch(__hidden_FBXModule::_IteratorThrower){}
}

template<typename NODE, typename FUNC>
static void FBXIterateBackwardNode(NODE* pNode, FUNC func){
    if(pNode){
        for(; pNode; pNode = pNode->Parent)
            func(pNode);
    }
}
template<typename NODE, typename FUNC>
static void FBXBreakableIterateBackwardNode(NODE* pNode, FUNC func){
    if(pNode){
        for(; pNode; pNode = pNode->Parent){
            if(!func(pNode))
                return;
        }
    }
}

template<typename NODE>
static inline NODE*& FBXFindLastAddible(NODE*& pNode){
    if(!pNode)
        return pNode;

    auto** pTarget = &pNode->Sibling;
    for(; (*pTarget); pTarget = &(*pTarget)->Sibling);

    return *pTarget;
}


#endif // _FBXUTILITES_HPP_
