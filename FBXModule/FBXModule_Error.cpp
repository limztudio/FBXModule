/**
* @file FBXModule_Error.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <FBXModule.hpp>

#include "FBXShared.h"


__FBXM_MAKE_FUNC(int, FBXGetLastError, char* szMessage){
    if(shr_errorStack.empty())
        return -1;

    const auto& lastMessage = shr_errorStack.top();
    auto messageLenth = (int)lastMessage.length();

    if(!messageLenth)
        return 0;

    if(szMessage){
        CopyMemory(szMessage, lastMessage.c_str(), messageLenth);
        szMessage[messageLenth] = 0;

        shr_errorStack.pop();
    }

    return messageLenth + 1;
}
