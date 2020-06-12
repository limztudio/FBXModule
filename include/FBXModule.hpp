/**
* @file FBXModule.hpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#ifndef _FBXMODULE_HPP_
#define _FBXMODULE_HPP_


#include "FBXConfig.hpp"

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

#define __FBXM_MAKE_FUNC(type, name, ...) extern type (__FBXM_CALL_TYPE *name) (__VA_ARGS__)

#endif


#include "FBXModuleDecl.hpp"


#ifndef __FBXM_DLL_EXPORT

#undef __FBXM_MAKE_FUNC

#endif


#endif // _FBXMODULE_HPP_
