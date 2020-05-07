/**
* @file def.h
* @date 2020/05/07
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include <malloc.h>
#include <EASTL/algorithm.h>


template<typename T, typename... ARGS>
static inline T* newC(ARGS&&... args){
    static const size_t size = sizeof(T);

    void* ptr = malloc(size);

    if(ptr){
        auto* _new = reinterpret_cast<T*>(ptr);

        ::new(_new) T(eastl::forward<ARGS>(args)...);

        return _new;
    }

    throw std::bad_alloc();
    return nullptr;
}

template<typename T>
static inline void deleteC(T* obj){
    obj->~T();
    free(obj);
}