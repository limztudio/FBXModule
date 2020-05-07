/**
* @file FBXType.hpp
* @date 2019/10/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include <malloc.h>


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
class FBXDynamicArray{
public:
    FBXDynamicArray()
        :
        Length(0),
        Values(nullptr)
    {}
    virtual ~FBXDynamicArray(){
        Clear();
    }


public:
    inline void Assign(FBX_SIZE length){
        if(Values)
            free(Values);

        Length = length;
        if(Length){
            Values = reinterpret_cast<T*>(malloc(Length * sizeof(T)));
            for(auto* p = Values; FBX_PTRDIFFU(p - Values) < Length; ++p)
                ::new(p) T();
        }
    }
    inline void Clear(){
        if(Values)
            free(Values);

        Length = 0;
        Values = nullptr;
    }


public:
    FBX_SIZE Length;
    T* Values;
};


template<typename T, unsigned long LEN>
struct FBXStaticArray{
    T Values[LEN];
};