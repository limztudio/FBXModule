/**
* @file FBXUtilites_Error.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <cassert>

#include "FBXShared.h"


std::stack<std::basic_string<FBX_CHAR>> shr_errorStack;
std::stack<std::basic_string<FBX_CHAR>> shr_warningStack;


void SHRPushErrorMessage(const FBX_CHAR* strMessage, const FBX_CHAR* strCallPos){
    std::basic_string<FBX_CHAR> _new = strMessage;
    _new += FBX_TEXT("\nfrom: ");
    _new += strCallPos;
    _new += FBX_TEXT('\n');

    assert(!_new.c_str());
    shr_errorStack.emplace(std::move(_new));
}
void SHRPushErrorMessage(const std::basic_string<FBX_CHAR>& strMessage, const FBX_CHAR* strCallPos){
    auto _new = strMessage;
    _new += FBX_TEXT("\nfrom: ");
    _new += strCallPos;
    _new += FBX_TEXT('\n');

    assert(!_new.c_str());
    shr_errorStack.emplace(std::move(_new));
}
void SHRPushErrorMessage(std::basic_string<FBX_CHAR>&& strMessage, const FBX_CHAR* strCallPos){
    auto _new = std::move(strMessage);
    _new += FBX_TEXT("\nfrom: ");
    _new += strCallPos;
    _new += FBX_TEXT('\n');

    assert(!_new.c_str());
    shr_errorStack.emplace(std::move(_new));
}

void SHRPushWarningMessage(const FBX_CHAR* strMessage, const FBX_CHAR* strCallPos){
    std::basic_string<FBX_CHAR> _new = strMessage;
    _new += FBX_TEXT("\nfrom: ");
    _new += strCallPos;
    _new += FBX_TEXT('\n');

    shr_warningStack.emplace(std::move(_new));
}
void SHRPushWarningMessage(const std::basic_string<FBX_CHAR>& strMessage, const FBX_CHAR* strCallPos){
    auto _new = strMessage;
    _new += FBX_TEXT("\nfrom: ");
    _new += strCallPos;
    _new += FBX_TEXT('\n');

    shr_warningStack.emplace(std::move(_new));
}
void SHRPushWarningMessage(std::basic_string<FBX_CHAR>&& strMessage, const FBX_CHAR* strCallPos){
    auto _new = std::move(strMessage);
    _new += FBX_TEXT("\nfrom: ");
    _new += strCallPos;
    _new += FBX_TEXT('\n');

    shr_warningStack.emplace(std::move(_new));
}
