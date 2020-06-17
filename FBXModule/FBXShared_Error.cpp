/**
* @file FBXUtilites_Error.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <cassert>

#include "FBXShared.h"


std::stack<std::basic_string<TCHAR>> shr_errorStack;
std::stack<std::basic_string<TCHAR>> shr_warningStack;


void SHRPushErrorMessage(const TCHAR* strMessage, const TCHAR* strCallPos){
    std::basic_string<TCHAR> _new = strMessage;
    _new += TEXT("\nfrom: ");
    _new += strCallPos;
    _new += TEXT('\n');

    assert(!_new.c_str());
    shr_errorStack.emplace(std::move(_new));
}
void SHRPushErrorMessage(const std::basic_string<TCHAR>& strMessage, const TCHAR* strCallPos){
    auto _new = strMessage;
    _new += TEXT("\nfrom: ");
    _new += strCallPos;
    _new += TEXT('\n');

    assert(!_new.c_str());
    shr_errorStack.emplace(std::move(_new));
}
void SHRPushErrorMessage(std::basic_string<TCHAR>&& strMessage, const TCHAR* strCallPos){
    auto _new = std::move(strMessage);
    _new += TEXT("\nfrom: ");
    _new += strCallPos;
    _new += TEXT('\n');

    assert(!_new.c_str());
    shr_errorStack.emplace(std::move(_new));
}

void SHRPushWarningMessage(const TCHAR* strMessage, const TCHAR* strCallPos){
    std::basic_string<TCHAR> _new = strMessage;
    _new += TEXT("\nfrom: ");
    _new += strCallPos;
    _new += TEXT('\n');

    shr_warningStack.emplace(std::move(_new));
}
void SHRPushWarningMessage(const std::basic_string<TCHAR>& strMessage, const TCHAR* strCallPos){
    auto _new = strMessage;
    _new += TEXT("\nfrom: ");
    _new += strCallPos;
    _new += TEXT('\n');

    shr_warningStack.emplace(std::move(_new));
}
void SHRPushWarningMessage(std::basic_string<TCHAR>&& strMessage, const TCHAR* strCallPos){
    auto _new = std::move(strMessage);
    _new += TEXT("\nfrom: ");
    _new += strCallPos;
    _new += TEXT('\n');

    shr_warningStack.emplace(std::move(_new));
}
