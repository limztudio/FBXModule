/**
* @file FBXModule_Compatible.cpp
* @date 2020/05/14
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <FBXModule.hpp>
#include "FBXMath.h"
#include "FBXShared.h"


__FBXM_MAKE_FUNC(bool, FBXCheckCompatibility, void){
    static const char __name_of_this_func[] = "FBXCheckCompatibility(void)";

    if(!SIMDCompetible()){
        SHRPushErrorMessage("this CPU doesn't support FMA or AVX instruction", __name_of_this_func);
        return false;
    }

    return true;
}
