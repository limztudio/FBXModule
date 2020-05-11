/**
* @file FBXModule.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include "FBXShared.h"


__FBXM_MAKE_FUNC(void*, FBXGetRoot, void){
    return shr_root;
}
__FBXM_MAKE_FUNC(void, FBXCopyRoot, void* dest, const void* src){
    auto* dest_c = static_cast<FBXRoot*>(dest);
    const auto* src_c = static_cast<const FBXRoot*>(src);

    SHRCopyRoot(dest_c, src_c);
}