﻿/**
 * @file FBXUtilites_Error.cpp
 * @date 2018/06/15
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#include "stdafx.h"

#include <cassert>

#include "FBXShared.h"


fbx_stack<fbx_string> shr_errorStack;
fbx_stack<fbx_string> shr_warningStack;


void SHRPushErrorMessage(const FBX_CHAR* strMessage, const FBX_CHAR* strCallPos){
    if(!strMessage){
        FBXM_ASSERT(strMessage != nullptr);
        return;
    }

    fbx_string _new = strMessage;
    _new += FBX_TEXT("\nfrom: ");
    _new += strCallPos;
    _new += FBX_TEXT("\n");

#ifdef _DEBUG
    if(!_new.c_str()){
        FBXM_ASSERT(_new.c_str() != nullptr);
        return;
    }
#endif
    shr_errorStack.emplace(std::move(_new));
}
void SHRPushErrorMessage(const fbx_string& strMessage, const FBX_CHAR* strCallPos){
    if(strMessage.empty()){
        FBXM_ASSERT(strMessage.empty() == false);
        return;
    }

    auto _new = strMessage;
    _new += FBX_TEXT("\nfrom: ");
    _new += strCallPos;
    _new += FBX_TEXT("\n");

#ifdef _DEBUG
    if(!_new.c_str()){
        FBXM_ASSERT(_new.c_str() != nullptr);
        return;
    }
#endif
    shr_errorStack.emplace(std::move(_new));
}
void SHRPushErrorMessage(fbx_string&& strMessage, const FBX_CHAR* strCallPos){
    if(strMessage.empty()){
        FBXM_ASSERT(strMessage.empty() == false);
        return;
    }

    auto _new = std::move(strMessage);
    _new += FBX_TEXT("\nfrom: ");
    _new += strCallPos;
    _new += FBX_TEXT("\n");

#ifdef _DEBUG
    if(!_new.c_str()){
        FBXM_ASSERT(_new.c_str() != nullptr);
        return;
    }
#endif
    shr_errorStack.emplace(std::move(_new));
}

void SHRPushWarningMessage(const FBX_CHAR* strMessage, const FBX_CHAR* strCallPos){
    if(!strMessage){
        FBXM_ASSERT(strMessage != nullptr);
        return;
    }

    fbx_string _new = strMessage;
    _new += FBX_TEXT("\nfrom: ");
    _new += strCallPos;
    _new += FBX_TEXT("\n");

#ifdef _DEBUG
    if(!_new.c_str()){
        FBXM_ASSERT(_new.c_str() != nullptr);
        return;
    }
#endif
    shr_warningStack.emplace(std::move(_new));
}
void SHRPushWarningMessage(const fbx_string& strMessage, const FBX_CHAR* strCallPos){
    if(strMessage.empty()){
        FBXM_ASSERT(strMessage.empty() == false);
        return;
    }

    auto _new = strMessage;
    _new += FBX_TEXT("\nfrom: ");
    _new += strCallPos;
    _new += FBX_TEXT("\n");

#ifdef _DEBUG
    if(!_new.c_str()){
        FBXM_ASSERT(_new.c_str() != nullptr);
        return;
    }
#endif
    shr_warningStack.emplace(std::move(_new));
}
void SHRPushWarningMessage(fbx_string&& strMessage, const FBX_CHAR* strCallPos){
    if(strMessage.empty()){
        FBXM_ASSERT(strMessage.empty() == false);
        return;
    }

    auto _new = std::move(strMessage);
    _new += FBX_TEXT("\nfrom: ");
    _new += strCallPos;
    _new += FBX_TEXT("\n");

#ifdef _DEBUG
    if(!_new.c_str()){
        FBXM_ASSERT(_new.c_str() != nullptr);
        return;
    }
#endif
    shr_warningStack.emplace(std::move(_new));
}
