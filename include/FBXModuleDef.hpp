/**
* @file FBXModuleDef.hpp
* @date 2020/05/27
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#ifndef __FBXM_DLL_EXPORT


#include "FBXModulePreDef.hpp"


#define __FBXM_MAKE_FUNC(type, name, ...) type (__FBXM_CALL_TYPE *name) (__VA_ARGS__) = nullptr;


#include "FBXModuleDecl.hpp"


#undef __FBXM_MAKE_FUNC

#endif
