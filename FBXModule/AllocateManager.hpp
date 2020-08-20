/**
* @file AllocateManager.hpp
* @date 2020/08/20
* @author Lim Taewoo (limztudio@gmail.com)
*/


#ifndef _ALLOCATEMANAGER_HPP_
#define _ALLOCATEMANAGER_HPP_


#include <cassert>

#include <malloc.h>


size_t dynamicAllocCount = 0u;


void* FBX_ALLOC(size_t size){
    if(!size){
        assert(("ALLOCATION FAILED: tried to allocate 0.", size != 0u));
        return nullptr;
    }

    auto* ptr = malloc(size);
    if(!ptr){
        assert(("ALLOCATION FAILED: failed to allocate.", ptr != nullptr));
        return nullptr;
    }

    ++dynamicAllocCount;
    return ptr;
}
void FBX_FREE(void* object){
    if(!dynamicAllocCount){
        assert(("ALLOCATION FAILED: object must be allocated through 'FBX_ALLOC'.", dynamicAllocCount != 0u));
        return;
    }

    if(!object){
        assert(("ALLOCATION FAILED: object must not be null.", object != nullptr));
        return;
    }

    free(object);
    --dynamicAllocCount;
}


#endif // _ALLOCATEMANAGER_HPP_

