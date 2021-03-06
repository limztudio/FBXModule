/**
 * @file FBXModule.hpp
 * @date 2018/06/15
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#ifndef _FBXMODULE_HPP_
#define _FBXMODULE_HPP_


#ifndef __FBXM_DLL_EXPORT

#include <malloc.h>
#define FBXM_ALLOC malloc
#define FBXM_FREE free

#endif

#include "FBXConfig.hpp"

#include "FBXType.hpp"

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

#define __FBXM_MAKE_HIDDEN_FUNC(type, fid, name, ...) __FBXM_MAKE_FUNC(type, name, __VA_ARGS__)


#include "FBXModuleDecl.hpp"


#ifndef __FBXM_DLL_EXPORT

#undef __FBXM_MAKE_FUNC
#undef __FBXM_MAKE_HIDDEN_FUNC

#endif


#ifndef __FBXM_DLL_EXPORT

#include "FBXUtilites_dependent.hpp"

#endif


#endif // _FBXMODULE_HPP_
