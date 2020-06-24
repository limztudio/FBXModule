/**
* @file FBXShared.cpp
* @date 2020/05/08
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <unordered_map>

#include "FBXUtilites.h"
#include "FBXShared.h"


FBXIOSetting shr_ioSetting;

FBXRoot* shr_root = nullptr;


void SHRCreateRoot(){
    if(shr_root)
        FBXDelete(shr_root);

    shr_root = FBXNew<FBXRoot>();
}
void SHRDeleteRoot(){
    if(shr_root){
        FBXDelete(shr_root);
        shr_root = nullptr;
    }
}
