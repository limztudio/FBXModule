/**
* @file FBXShared.cpp
* @date 2020/05/08
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include "FBXUtilites.h"
#include "FBXShared.h"


FBXRoot* shr_root = nullptr;


void SHRCreateRoot(){
    SHRDeleteRoot();

    shr_root = FBXNew<FBXRoot>();
}
void SHRDeleteRoot(){
    if(shr_root){
        FBXDelete(shr_root);
        shr_root = nullptr;
    }
}