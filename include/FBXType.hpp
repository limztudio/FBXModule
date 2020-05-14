/**
* @file FBXType.hpp
* @date 2019/10/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXAssign.hpp"


enum class FBXIOType : unsigned long{
    FBXIOType_None = 0,

    FBXIOType_BinaryExport = 1,
};


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
        Values(rhs.Length ? FBXAllocate<T>(rhs.Length) : nullptr)
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
        if(Values){
            for(auto* p = Values; FBX_PTRDIFFU(p - Values) < Length; ++p)
                p->~T();

            FBXFree(Values);
        }
    }


public:
    FBXDynamicArray<T>& operator=(const FBXDynamicArray<T>& rhs){
        if(Length != rhs.Length){
            Length = rhs.Length;
            if(Values){
                for(auto* p = Values; FBX_PTRDIFFU(p - Values) < Length; ++p)
                    p->~T();

                FBXFree(Values);
            }

            if(Length){
                Values = FBXAllocate<T>(Length);

                auto* d = Values;
                const auto* s = rhs.Values;
                for(; FBX_PTRDIFFU(s - rhs.Values) < rhs.Length; ++s, ++d)
                    ::new(d) T(*s);
            }
            else
                Values = nullptr;
        }
        else{
            auto* d = Values;
            const auto* s = rhs.Values;
            for(; FBX_PTRDIFFU(s - rhs.Values) < rhs.Length; ++s, ++d)
                (*d) = (*s);
        }

        return *this;
    }
    FBXDynamicArray<T>& operator=(FBXDynamicArray<T>&& rhs){
        if(Values){
            for(auto* p = Values; FBX_PTRDIFFU(p - Values) < Length; ++p)
                p->~T();

            FBXFree(Values);
        }

        Length = rhs.Length;
        Values = rhs.Values;

        rhs.Length = 0;
        rhs.Values = nullptr;

        return *this;
    }


public:
    inline void Assign(FBX_SIZE length){
        if(Values){
            for(auto* p = Values; FBX_PTRDIFFU(p - Values) < Length; ++p)
                p->~T();

            FBXFree(Values);
        }

        Length = length;
        if(Length){
            Values = FBXAllocate<T>(Length);
            for(auto* p = Values; FBX_PTRDIFFU(p - Values) < Length; ++p)
                ::new(p) T();
        }
    }
    inline void Clear(){
        if(Values){
            for(auto* p = Values; FBX_PTRDIFFU(p - Values) < Length; ++p)
                p->~T();

            FBXFree(Values);
        }

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

        return *this;
    }


public:
    static const FBX_SIZE Length = LEN;
    T Values[LEN];
};


template<typename T>
static inline bool FBXTypeHasMember(T target, T find){
    const auto t = (unsigned long)target;
    const auto f = (unsigned long)find;

    return (t & f) == f;
}