/**
* @file stdafx.h
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <fbxsdk.h>


extern void* FBX_ALLOC(size_t size);
extern void FBX_FREE(void* object);
