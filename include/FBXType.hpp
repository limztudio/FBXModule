/**
* @file FBXType.hpp
* @date 2019/10/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXAssign.hpp"


template<typename T>
class FBXDynamicArray{
public:
    FBXDynamicArray()
        :
        Length(0),
        Values(nullptr)
    {}
    FBXDynamicArray(const FBXDynamicArray<T>& rhs)
        :
        Length(rhs.Length),
        Values(FBXAllocate<T>(rhs.Length))
    {
        auto* d = Values;
        const auto* s = rhs.Values;
        for(; FBX_PTRDIFFU(s - rhs.Values) < rhs.Length; ++s, ++d)
            ::new(d) T(*s);
    }
    FBXDynamicArray(FBXDynamicArray<T>&& rhs)
        :
        Length(rhs.Length),
        Values(rhs.Values)
    {
        rhs.Length = 0;
        rhs.Values = nullptr;
    }

    virtual ~FBXDynamicArray(){
        Clear();
    }


public:
    FBXDynamicArray<T>& operator=(const FBXDynamicArray<T>& rhs){
        if(Length != rhs.Length){
            Length = rhs.Length;
            if(Values){
                FBXFree(Values);
                Values = FBXAllocate<T>(Length);
            }
        }

        if(Length){
            auto* d = Values;
            const auto* s = rhs.Values;
            for(; FBX_PTRDIFFU(s - rhs.Values) < rhs.Length; ++s, ++d)
                ::new(d) T(*s);
        }
    }
    FBXDynamicArray<T>& operator=(FBXDynamicArray<T>&& rhs){
        if(Values)
            FBXFree(Values);

        Length = rhs.Length;
        Values = rhs.Values;

        rhs.Length = 0;
        rhs.Values = nullptr;
    }


public:
    inline void Assign(FBX_SIZE length){
        if(Values)
            FBXFree(Values);

        Length = length;
        if(Length){
            Values = FBXAllocate<T>(Length);
            for(auto* p = Values; FBX_PTRDIFFU(p - Values) < Length; ++p)
                ::new(p) T();
        }
    }
    inline void Clear(){
        if(Values)
            FBXFree(Values);

        Length = 0;
        Values = nullptr;
    }


public:
    FBX_SIZE Length;
    T* Values;
};


template<typename T, unsigned long LEN>
class FBXStaticArray{
public:
    FBXStaticArray(){}
    FBXStaticArray(const FBXStaticArray<T, LEN>& rhs){
        auto* d = Values;
        const auto* s = rhs.Values;
        for(; FBX_PTRDIFFU(s - rhs.Values) < rhs.Length; ++s, ++d)
            (*d) = (*s);
    }


public:
    FBXStaticArray<T, LEN>& operator=(const FBXStaticArray<T, LEN>& rhs){
        auto* d = Values;
        const auto* s = rhs.Values;
        for(; FBX_PTRDIFFU(s - rhs.Values) < rhs.Length; ++s, ++d)
            (*d) = (*s);
    }


public:
    static const FBX_SIZE Length = LEN;
    T Values[LEN];
};