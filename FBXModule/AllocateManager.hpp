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
size_t dynamicAlignAllocCount = 0u;


void* FBXM_ALLOC(std::size_t size){
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
void* FBXM_ALIGN_ALLOC(std::size_t size, std::align_val_t align){
    if(!size){
        FBXM_ASSERT(("ALIGNED ALLOCATION FAILED: tried to allocate 0.", size != 0u));
        return nullptr;
    }

    auto* ptr = je_aligned_alloc(std::size_t(align), size);
    if(!ptr){
        FBXM_ASSERT(("ALIGNED ALLOCATION FAILED: failed to allocate.", ptr != nullptr));
        return nullptr;
    }

    ++dynamicAlignAllocCount;
    return ptr;
}

void FBXM_FREE(void* object){
    if(!dynamicAllocCount){
        FBXM_ASSERT(("DEALLOCATION FAILED: object must be allocated through 'FBXM_ALLOC'.", dynamicAllocCount != 0u));
        return;
    }

    if(!object){
        FBXM_ASSERT(("DEALLOCATION FAILED: object must not be null.", object != nullptr));
        return;
    }

    je_free(object);
    --dynamicAllocCount;
}
void FBXM_ALIGN_FREE(void* object){
    if(!dynamicAlignAllocCount){
        FBXM_ASSERT(("ALIGNED DEALLOCATION FAILED: object must be allocated through 'FBXM_ALIGN_ALLOC'.", dynamicAlignAllocCount != 0u));
        return;
    }

    if(!object){
        FBXM_ASSERT(("ALIGNED DEALLOCATION FAILED: object must not be null.", object != nullptr));
        return;
    }

    je_free(object);
    --dynamicAlignAllocCount;
}


void* operator new(std::size_t count){
    auto* ptr = FBXM_ALLOC(count);
    if(!ptr)
        throw std::bad_alloc{};
    return ptr;
}
void* operator new[](std::size_t count){
    auto* ptr = FBXM_ALLOC(count);
    if(!ptr)
        throw std::bad_alloc{};
    return ptr;
}
void* operator new(std::size_t count, const std::nothrow_t&)noexcept{
    return FBXM_ALLOC(count);
}
void* operator new[](std::size_t count, const std::nothrow_t&)noexcept{
    return FBXM_ALLOC(count);
}

void* operator new(std::size_t count, std::align_val_t al){
    auto* ptr = FBXM_ALIGN_ALLOC(count, al);
    if(!ptr)
        throw std::bad_alloc{};
    return ptr;
}
void* operator new[](std::size_t count, std::align_val_t al){
    auto* ptr = FBXM_ALIGN_ALLOC(count, al);
    if(!ptr)
        throw std::bad_alloc{};
    return ptr;
}
void* operator new(std::size_t count, std::align_val_t al, const std::nothrow_t&)noexcept{
    return FBXM_ALIGN_ALLOC(count, al);
}
void* operator new[](std::size_t count, std::align_val_t al, const std::nothrow_t&)noexcept{
    return FBXM_ALIGN_ALLOC(count, al);
}


void operator delete(void* ptr)noexcept{
    FBXM_FREE(ptr);
}
void operator delete[](void* ptr)noexcept{
    FBXM_FREE(ptr);
}
void operator delete(void* ptr, std::size_t sz)noexcept{
    FBXM_FREE(ptr);
}
void operator delete[](void* ptr, std::size_t sz)noexcept{
    FBXM_FREE(ptr);
}
void operator delete(void* ptr, const std::nothrow_t&)noexcept{
    FBXM_FREE(ptr);
}
void operator delete[](void* ptr, const std::nothrow_t&)noexcept{
    FBXM_FREE(ptr);
}

void operator delete(void* ptr, std::align_val_t al)noexcept{
    FBXM_ALIGN_FREE(ptr);
}
void operator delete[](void* ptr, std::align_val_t al)noexcept{
    FBXM_ALIGN_FREE(ptr);
}
void operator delete(void* ptr, std::size_t sz, std::align_val_t al)noexcept{
    FBXM_ALIGN_FREE(ptr);
}
void operator delete[](void* ptr, std::size_t sz, std::align_val_t al)noexcept{
    FBXM_ALIGN_FREE(ptr);
}
void operator delete(void* ptr, std::align_val_t al, const std::nothrow_t&)noexcept{
    FBXM_ALIGN_FREE(ptr);
}
void operator delete[](void* ptr, std::align_val_t al, const std::nothrow_t&)noexcept{
    FBXM_ALIGN_FREE(ptr);
}


#endif // _ALLOCATEMANAGER_HPP_

