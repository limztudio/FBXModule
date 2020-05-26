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
enum class FBXAxisSystem : unsigned long{
    FBXAxisSystem_UpVector_XAxis = 1 << 0,
    FBXAxisSystem_UpVector_NegXAxis = 1 << 1,
    FBXAxisSystem_UpVector_YAxis = 1 << 2,
    FBXAxisSystem_UpVector_NegYAxis = 1 << 3,
    FBXAxisSystem_UpVector_ZAxis = 1 << 4,
    FBXAxisSystem_UpVector_NegZAxis = 1 << 5,

    FBXAxisSystem_UpVector_Mask = (
    FBXAxisSystem_UpVector_XAxis
    | FBXAxisSystem_UpVector_NegXAxis
    | FBXAxisSystem_UpVector_YAxis
    | FBXAxisSystem_UpVector_NegYAxis
    | FBXAxisSystem_UpVector_ZAxis
    | FBXAxisSystem_UpVector_NegZAxis
    ),


    FBXAxisSystem_FrontVector_ParityEven = 1 << 6,
    FBXAxisSystem_FrontVector_NegParityEven = 1 << 7,
    FBXAxisSystem_FrontVector_ParityOdd = 1 << 8,
    FBXAxisSystem_FrontVector_NegParityOdd = 1 << 9,

    FBXAxisSystem_FrontVector_Mask = (
    FBXAxisSystem_FrontVector_ParityEven
    | FBXAxisSystem_FrontVector_NegParityEven
    | FBXAxisSystem_FrontVector_ParityOdd
    | FBXAxisSystem_FrontVector_NegParityOdd
    ),


    FBXAxisSystem_CoordSystem_LeftHanded = 1 << 10,
    FBXAxisSystem_CoordSystem_RightHanded = 1 << 11,

    FBXAxisSystem_CoordSystem_Mask = (
    FBXAxisSystem_CoordSystem_LeftHanded
    | FBXAxisSystem_CoordSystem_RightHanded
    ),


    FBXAxisSystem_Preset_Maya_Zup = (FBXAxisSystem_UpVector_ZAxis | FBXAxisSystem_FrontVector_NegParityOdd | FBXAxisSystem_CoordSystem_RightHanded),
    FBXAxisSystem_Preset_Maya_Yup = (FBXAxisSystem_UpVector_YAxis | FBXAxisSystem_FrontVector_ParityOdd | FBXAxisSystem_CoordSystem_RightHanded),
    FBXAxisSystem_Preset_Max = (FBXAxisSystem_UpVector_ZAxis | FBXAxisSystem_FrontVector_NegParityOdd | FBXAxisSystem_CoordSystem_RightHanded),
    FBXAxisSystem_Preset_MotionBuilder = (FBXAxisSystem_UpVector_YAxis | FBXAxisSystem_FrontVector_ParityOdd | FBXAxisSystem_CoordSystem_RightHanded),
    FBXAxisSystem_Preset_OpenGL = (FBXAxisSystem_UpVector_YAxis | FBXAxisSystem_FrontVector_ParityOdd | FBXAxisSystem_CoordSystem_RightHanded),
    FBXAxisSystem_Preset_DirectX = (FBXAxisSystem_UpVector_YAxis | FBXAxisSystem_FrontVector_ParityOdd | FBXAxisSystem_CoordSystem_LeftHanded),
    FBXAxisSystem_Preset_Lightwave = (FBXAxisSystem_UpVector_YAxis | FBXAxisSystem_FrontVector_ParityOdd | FBXAxisSystem_CoordSystem_LeftHanded),
};

class FBXIOSetting{
public:
    FBXIOSetting()
        :
        MaxParticipateClusterPerVertex(4),
        MaxBoneCountPerMesh(20),

        AxisSystem(FBXAxisSystem::FBXAxisSystem_Preset_DirectX),
        UnitScale(2.54),
        UnitMultiplier(1.)
    {}


public:
    unsigned long MaxParticipateClusterPerVertex;
    unsigned long MaxBoneCountPerMesh;

public:
    FBXAxisSystem AxisSystem;
    double UnitScale;
    double UnitMultiplier;
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