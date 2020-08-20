/**
* @file FBXAssign.hpp
* @date 2020/05/08
* @author Lim Taewoo (limztudio@gmail.com)
*/


#ifndef _FBXASSIGN_HPP_
#define _FBXASSIGN_HPP_


#ifdef _WIN64
typedef __int64 FBX_PTRDIFF;
typedef unsigned __int64 FBX_PTRDIFFU;
typedef unsigned __int64 FBX_SIZE;
#else
typedef int FBX_PTRDIFF;
typedef unsigned int FBX_PTRDIFFU;
typedef unsigned long FBX_SIZE;
#endif


template<typename T>
static inline T* FBXAllocate(FBX_SIZE len){
    return reinterpret_cast<T*>(FBX_ALLOC(len * sizeof(T)));
}

static inline void FBXFree(void* obj){
    FBX_FREE(obj);
}


template<typename T, typename... ARGS>
static inline T* FBXNew(ARGS&&... args){
    static const FBX_SIZE size = sizeof(T);

    void* ptr = FBX_ALLOC(size);
    if(ptr){
        auto* _new = reinterpret_cast<T*>(ptr);
        ::new(_new) T(std::forward<ARGS>(args)...);
        return _new;
    }

    throw std::bad_alloc();
    return nullptr;
}

template<typename T>
static inline void FBXDelete(T* obj){
    obj->~T();
    FBX_FREE(obj);
}


#endif // _FBXASSIGN_HPP_
