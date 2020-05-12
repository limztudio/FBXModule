/**
* @file FBXUtilites.hpp
* @date 2020/05/08
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXType.hpp"


namespace FBXUtilites{
    template<typename T>
    static FBX_SIZE Memlen(const T* p){
        FBX_SIZE i = 0;
        for(; (*p); ++p, ++i);
        return i;
    }

    template<typename NODE, typename FUNC>
    static void IterateAnimation(NODE* p, FUNC func){
        if(p){
            IterateAnimation(p->Next, func);
            func(p);
        }
    }
    template<typename NODE, typename FUNC>
    static void IterateNode(NODE* p, FUNC func){
        if(p){
            IterateNode(p->Child, func);
            IterateNode(p->Sibling, func);
            func(p);
        }
    }
};
