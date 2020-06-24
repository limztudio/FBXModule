/**
* @file FBXModule_Copy.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include "FBXShared.h"


__FBXM_MAKE_FUNC(void, __hidden_FBXModule_RebindRoot, void* pDest, const void* pSrc){
    auto* dest = static_cast<FBXRoot*>(pDest);
    const auto* src = static_cast<const FBXRoot*>(pSrc);

    SHRRebindRoot(dest, src);
}
