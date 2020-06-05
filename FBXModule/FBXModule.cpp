/**
* @file FBXModule.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include "FBXShared.h"


__FBXM_MAKE_FUNC(const void*, FBXGetRoot, void){
    return shr_root;
}
__FBXM_MAKE_FUNC(void, FBXCopyRoot, void* pDest, const void* pSrc){
    auto* dest = static_cast<FBXRoot*>(pDest);
    const auto* src = static_cast<const FBXRoot*>(pSrc);

    SHRCopyRoot(dest, src);
}
