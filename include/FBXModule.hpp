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


#include "FBXModulePreDef.hpp"


#ifdef __FBXM_DLL_EXPORT

#define __FBXM_MAKE_FUNC(type, name, ...) extern "C" __declspec(dllexport) type __FBXM_CALL_TYPE name (__VA_ARGS__)

#else

#define __FBXM_MAKE_FUNC(type, name, ...) extern type (__FBXM_CALL_TYPE *name) (__VA_ARGS__);

#define __FBXM_BIND_FUNC(lib, name) name = reinterpret_cast<decltype(name)>(GetProcAddress(lib, #name));

#endif


#include "FBXModuleDecl.hpp"


#ifndef __FBXM_DLL_EXPORT

#undef __FBXM_MAKE_FUNC

#endif
