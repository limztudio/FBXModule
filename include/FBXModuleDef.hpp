/**
 * @file FBXModuleDef.hpp
 * @date 2020/05/27
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#ifndef __FBXM_DLL_EXPORT


#include "FBXModulePreDef.hpp"


#define __FBXM_MAKE_FUNC(type, name, ...) type (__FBXM_CALL_TYPE *name) (__VA_ARGS__) = nullptr
#define __FBXM_MAKE_HIDDEN_FUNC(type, fid, name, ...) __FBXM_MAKE_FUNC(type, name, __VA_ARGS__)


#include "FBXModuleDecl.hpp"


#undef __FBXM_MAKE_FUNC
#undef __FBXM_MAKE_HIDDEN_FUNC

#endif
