/**
* @file AllocateManager.hpp
* @date 2020/08/20
* @author Lim Taewoo (limztudio@gmail.com)
*/


#ifndef _ALLOCATEMANAGER_HPP_
#define _ALLOCATEMANAGER_HPP_


#include <cassert>

#include <jemalloc/jemalloc.h>


size_t dynamicAllocCount = 0u;


void* FBX_ALLOC(size_t size){
    if(!size){
        FBXM_ASSERT(("ALLOCATION FAILED: tried to allocate 0.", size != 0u));
        return nullptr;
    }

    auto* ptr = je_malloc(size);
    if(!ptr){
        FBXM_ASSERT(("ALLOCATION FAILED: failed to allocate.", ptr != nullptr));
        return nullptr;
    }

    ++dynamicAllocCount;
    return ptr;
}
void FBX_FREE(void* object){
    if(!dynamicAllocCount){
        FBXM_ASSERT(("DEALLOCATION FAILED: object must be allocated through 'FBX_ALLOC'.", dynamicAllocCount != 0u));
        return;
    }

    if(!object){
        FBXM_ASSERT(("DEALLOCATION FAILED: object must not be null.", object != nullptr));
        return;
    }

    je_free(object);
    --dynamicAllocCount;
}


#endif // _ALLOCATEMANAGER_HPP_

