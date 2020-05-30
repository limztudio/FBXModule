/**
* @file FBXUtilites_Error.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <cassert>

#include "FBXShared.h"


std::stack<std::string> shr_errorStack;


void SHRPushErrorMessage(const char* strMessage, const char* strCallPos){
    std::string _new = strMessage;
    _new += "\nfrom: ";
    _new += strCallPos;
    _new += '\n';

    assert(!_new.c_str());
    shr_errorStack.emplace(std::move(_new));
}
void SHRPushErrorMessage(const std::string& strMessage, const char* strCallPos){
    auto _new = strMessage;
    _new += "\nfrom: ";
    _new += strCallPos;
    _new += '\n';

    assert(!_new.c_str());
    shr_errorStack.emplace(std::move(_new));
}
void SHRPushErrorMessage(std::string&& strMessage, const char* strCallPos){
    auto _new = std::move(strMessage);
    _new += "\nfrom: ";
    _new += strCallPos;
    _new += '\n';

    assert(!_new.c_str());
    shr_errorStack.emplace(std::move(_new));
}
