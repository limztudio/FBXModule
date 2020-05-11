/**
* @file FBXModule.hpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "FBXType.hpp"

#ifndef __FBXM_DLL_EXPORT
#include "FBXAllocate.hpp"
#endif

#include "FBXBase.hpp"

#include "FBXRoot.hpp"

#include "FBXNode.hpp"

#include "FBXBone.hpp"
#include "FBXMesh.hpp"
#include "FBXSkinnedMesh.hpp"

#include "FBXAnimation.hpp"


#define __FBXM_CALL_TYPE __cdecl


#ifdef __FBXM_DLL_EXPORT

#define __FBXM_MAKE_FUNC(type, name, ...) extern "C" __declspec(dllexport) type __FBXM_CALL_TYPE name (__VA_ARGS__)

#else

#define __FBXM_MAKE_FUNC(type, name, ...) type (__FBXM_CALL_TYPE *name) (__VA_ARGS__) = nullptr;

#define __FBXM_BIND_FUNC(lib, name) name = reinterpret_cast<decltype(name)>(GetProcAddress(lib, #name));

#endif


__FBXM_MAKE_FUNC(int, FBXGetLastError, char* szMessage);

__FBXM_MAKE_FUNC(bool, FBXOpenFile, const char* szfilePath, const char* mode);
__FBXM_MAKE_FUNC(bool, FBXCloseFile, void);

__FBXM_MAKE_FUNC(bool, FBXReadScene, void);

__FBXM_MAKE_FUNC(void*, FBXGetRoot, void);
__FBXM_MAKE_FUNC(void, FBXCopyRoot, void* dest, const void* src);