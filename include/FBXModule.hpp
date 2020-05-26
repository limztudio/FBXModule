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


__FBXM_MAKE_FUNC(bool, FBXCheckCompatibility, void);
__FBXM_MAKE_FUNC(unsigned long, FBXGetErrorCount, void);
__FBXM_MAKE_FUNC(int, FBXGetLastError, char* szMessage);

__FBXM_MAKE_FUNC(bool, FBXOpenFile, const char* szfilePath, const char* mode, unsigned long ioFlag, const void* ioSetting);
__FBXM_MAKE_FUNC(bool, FBXCloseFile, void);

__FBXM_MAKE_FUNC(bool, FBXReadScene, void);
__FBXM_MAKE_FUNC(bool, FBXWriteScene, const void* pRoot);

__FBXM_MAKE_FUNC(const void*, FBXGetRoot, void);
__FBXM_MAKE_FUNC(void, FBXCopyRoot, void* pDest, const void* pSrc);

__FBXM_MAKE_FUNC(void, FBXGetWorldMatrix, void* pOutMatrix, const void* pNode);
__FBXM_MAKE_FUNC(void, FBXTransformCoord, void* pOutVec3, const void* pVec3, const void* pMatrix);
__FBXM_MAKE_FUNC(void, FBXTransformNormal, void* pOutVec3, const void* pVec3, const void* pMatrix);
