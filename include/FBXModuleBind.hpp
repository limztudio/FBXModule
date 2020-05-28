/**
* @file FBXModuleBind.hpp
* @date 2020/05/28
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


template<typename LIBRARY>
static inline void FBXBindFunction(LIBRARY library){
#define __FBXM_MAKE_FUNC(type, name, ...) name = reinterpret_cast<decltype(name)>(GetProcAddress(library, #name))


#include "FBXModuleDecl.hpp"


#undef __FBXM_MAKE_FUNC
}
