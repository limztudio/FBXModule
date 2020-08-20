/**
* @file AllocateManager.hpp
* @date 2020/08/20
* @author Lim Taewoo (limztudio@gmail.com)
*/


#ifndef _ALLOCATEMANAGER_HPP_
#define _ALLOCATEMANAGER_HPP_


#include <malloc.h>


void* FBX_ALLOC(size_t size){
    return malloc(size);
}
void FBX_FREE(void* object){
    free(object);
}


#endif // _ALLOCATEMANAGER_HPP_

