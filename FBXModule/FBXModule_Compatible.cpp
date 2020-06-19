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
    static const FBX_CHAR __name_of_this_func[] = TEXT("FBXCheckCompatibility(void)");

    if(!SIMDCompetible()){
        SHRPushErrorMessage(
            TEXT("this CPU doesn't support one or more following instruction(s): ")
#if ((!defined(_SIMD_AVX)) && (!defined(_SIMD_FMA)))
            TEXT("SSE4")
#elif ((!defined(_SIMD_AVX)) && (defined(_SIMD_FMA)))
            TEXT("SSE4 and FMA3")
#elif ((defined(_SIMD_AVX)) && (!defined(_SIMD_FMA)))
            TEXT("SSE4 and AVX2")
#elif ((defined(_SIMD_AVX)) && (defined(_SIMD_FMA)))
            TEXT("SSE4, FMA3 and AVX2")
#endif
        , __name_of_this_func);
        return false;
    }

    return true;
}
