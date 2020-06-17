/**
* @file FBXModule_Error.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <FBXModule.hpp>

#include "FBXShared.h"


__FBXM_MAKE_FUNC(unsigned long, FBXGetErrorCount, void){
    auto count = shr_errorStack.size();

    return (unsigned long)(count);
}

__FBXM_MAKE_FUNC(int, FBXGetLastError, FBX_CHAR* szMessage){
    if(shr_errorStack.empty())
        return -1;

    const auto& lastMessage = shr_errorStack.top();
    auto messageLenth = (int)lastMessage.length();

    if(!messageLenth){
        shr_errorStack.pop();
        return 0;
    }

    if(szMessage){
        CopyMemory(szMessage, lastMessage.c_str(), messageLenth);
        szMessage[messageLenth] = 0;

        shr_errorStack.pop();
    }

    return messageLenth + 1;
}

__FBXM_MAKE_FUNC(unsigned long, FBXGetWarningCount, void){
    auto count = shr_warningStack.size();

    return (unsigned long)(count);
}

__FBXM_MAKE_FUNC(int, FBXGetLastWarning, FBX_CHAR* szMessage){
    if(shr_warningStack.empty())
        return -1;

    const auto& lastMessage = shr_warningStack.top();
    auto messageLenth = (int)lastMessage.length();

    if(!messageLenth){
        shr_warningStack.pop();
        return 0;
    }

    if(szMessage){
        CopyMemory(szMessage, lastMessage.c_str(), messageLenth);
        szMessage[messageLenth] = 0;

        shr_warningStack.pop();
    }

    return messageLenth + 1;
}
