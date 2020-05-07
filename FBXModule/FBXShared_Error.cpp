/**
* @file FBXUtilites_Error.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include "FBXShared.h"


eastl::stack<eastl::string> shr_errorStack;


void SHRPushErrorMessage(const char* strMessage, const char* strCallPos){
    eastl::string _new = strMessage;
    _new += "\nfrom: ";
    _new += strCallPos;
    _new += '\n';

    shr_errorStack.emplace(eastl::move(_new));
}