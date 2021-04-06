/**
 * @file FBXModule_Option.cpp
 * @date 2021/04/05
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#include "stdafx.h"

#include <FBXModule.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


__FBXM_MAKE_FUNC(bool, FBXReduceKeyframe, const FBX_CHAR** szExcludeNames, unsigned long uExcludeNameCount, unsigned char cMask, double fPrecision){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("FBXReduceKeyframe(const FBX_CHAR**, unsigned long, unsigned char, double)");


    if(!shr_scene){
        SHRPushErrorMessage(FBX_TEXT("scene must be opened before process"), __name_of_this_func);
        return false;
    }

    shr_tmpNodeNameList.clear();
    if((szExcludeNames != nullptr) && (uExcludeNameCount > 0)){
        shr_tmpNodeNameList.rehash(uExcludeNameCount << 2);
        for(auto i = decltype(uExcludeNameCount){ 0 }; i < uExcludeNameCount; ++i)
            shr_tmpNodeNameList.emplace(fbx_string(szExcludeNames[i]));
    }

    if(!SHRReduceAnimation(shr_SDKManager, shr_scene, shr_tmpNodeNameList, (TransformMask)cMask, fPrecision)){
        SHRPushErrorMessage(FBX_TEXT("failed reduce animations"), __name_of_this_func);
        return false;
    }

    return true;
}
