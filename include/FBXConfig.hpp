/**
 * @file FBXConfig.hpp
 * @date 2020/06/12
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#ifndef _FBXCONFIG_HPP_
#define _FBXCONFIG_HPP_


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
        ExportAsASCII(true),
        IgnoreAnimationIO(false),

        MaxParticipateClusterPerVertex(4),
        MaxBoneCountPerMesh(20),

        AxisSystem(FBXAxisSystem::FBXAxisSystem_Preset_DirectX),
        UnitScale(2.54),
        UnitMultiplier(1.),

        AnimationKeyCompareDifference(0.0001)
    {}


public:
    bool ExportAsASCII;
    bool IgnoreAnimationIO;

public:
    unsigned long MaxParticipateClusterPerVertex;
    unsigned long MaxBoneCountPerMesh;

public:
    FBXAxisSystem AxisSystem;
    double UnitScale;
    double UnitMultiplier;
    double AnimationKeyCompareDifference;
};


#endif // _FBXCONFIG_HPP_
