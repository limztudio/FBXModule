/**
* @file FBXUtilites_FbxSdk.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include "FBXShared.h"


using namespace fbxsdk;


//FbxAxisSystem shr_axisSystem(FbxAxisSystem::eZAxis, FbxAxisSystem::eParityEven, FbxAxisSystem::eLeftHanded);
//FbxSystemUnit shr_systemUnit(2.54);

FbxAxisSystem shr_axisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
FbxSystemUnit shr_systemUnit(1.);

FbxManager* shr_SDKManager = nullptr;

FbxScene* shr_scene = nullptr;


void SHRDestroyFbxSdkObjects(){
    if(shr_scene)
        shr_scene->Destroy(true);

    if(shr_SDKManager)
        shr_SDKManager->Destroy();

    shr_SDKManager = nullptr;

    shr_scene = nullptr;
}