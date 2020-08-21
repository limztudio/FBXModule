/**
* @file stdafx.h
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#pragma once


#include "targetver.h"

#include <string>
#include <vector>
#include <list>
#include <forward_list>
#include <deque>
#include <stack>
#include <queue>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <fbxsdk.h>


#ifndef FBXM_DECLSPEC_ALLOCATOR
#ifdef __clang__
#define FBXM_DECLSPEC_ALLOCATOR
#else
#define FBXM_DECLSPEC_ALLOCATOR	__declspec(allocator)
#endif
#endif


#define FBXM_ASSERT _ASSERTE


extern void* FBXM_ALLOC(std::size_t size);
extern void* FBXM_ALIGN_ALLOC(std::size_t size, std::align_val_t align);

extern void FBXM_FREE(void* object);
extern void FBXM_ALIGN_FREE(void* object);


template<class _Ty>
class FBXM_ALLOCATOR{
public:
    using _Not_user_specialized = void;

    using value_type = _Ty;

    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS typedef _Ty* pointer;
    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS typedef const _Ty* const_pointer;

    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS typedef _Ty& reference;
    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS typedef const _Ty& const_reference;

    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS typedef size_t size_type;
    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS typedef ptrdiff_t difference_type;

    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    template<class _Other>
    struct _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS rebind{
        // convert this type to allocator<_Other>
        using other = FBXM_ALLOCATOR<_Other>;
    };

    _NODISCARD _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS inline _Ty* address(_Ty& _Val)const noexcept{
        // return address of mutable _Val
        return (std::addressof(_Val));
    }

    _NODISCARD _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS inline const _Ty* address(const _Ty& _Val)const noexcept{
        // return address of nonmutable _Val
        return (std::addressof(_Val));
    }

    constexpr FBXM_ALLOCATOR()noexcept{}
    constexpr FBXM_ALLOCATOR(const FBXM_ALLOCATOR&)noexcept = default;
    template<class _Other>
    constexpr FBXM_ALLOCATOR(const FBXM_ALLOCATOR<_Other>&)noexcept{}

    inline void deallocate(_Ty* const _Ptr, const size_t _Count){
        // deallocate object at _Ptr
        // no overflow check on the following multiply; we assume _Allocate did that check
        FBXM_FREE(_Ptr);
    }

    _NODISCARD FBXM_DECLSPEC_ALLOCATOR inline _Ty* allocate(_CRT_GUARDOVERFLOW const size_t _Count){
        // allocate array of _Count elements
        void* ptr = FBXM_ALLOC(std::_Get_size_of_n<sizeof(_Ty)>(_Count));
        if(!ptr)
            throw std::bad_alloc();

        return reinterpret_cast<_Ty*>(ptr);
    }

    _NODISCARD _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS FBXM_DECLSPEC_ALLOCATOR inline _Ty* allocate(_CRT_GUARDOVERFLOW const size_t _Count, const void*){
        // allocate array of _Count elements, ignore hint
        return (allocate(_Count));
    }

    template<class _Objty, class... _Types>
    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS inline void construct(_Objty* const _Ptr, _Types&&... _Args){
        // construct _Objty(_Types...) at _Ptr
        ::new (const_cast<void*>(static_cast<const volatile void*>(_Ptr)))_Objty(std::forward<_Types>(_Args)...);
    }

    template<class _Uty>
    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS inline void destroy(_Uty* const _Ptr){
        // destroy object at _Ptr
        _Ptr->~_Uty();
    }

    _NODISCARD _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS inline size_t max_size()const noexcept{
        // estimate maximum array size
        return (static_cast<size_t>(-1) / sizeof(_Ty));
    }
};

// CLASS allocator<void>
template<>
class FBXM_ALLOCATOR<void>{
    // generic allocator for type void
public:
    using value_type = void;
    using pointer = void*;
    using const_pointer = const void*;

    template<class _Other>
    struct rebind{
        // convert this type to an allocator<_Other>
        using other = FBXM_ALLOCATOR<_Other>;
    };
};

template<class _Ty, class _Other>
_NODISCARD inline bool operator==(const FBXM_ALLOCATOR<_Ty>&, const FBXM_ALLOCATOR<_Other>&)noexcept{
    // test for allocator equality
    return (true);
}

template<class _Ty, class _Other>
_NODISCARD inline bool operator!=(const FBXM_ALLOCATOR<_Ty>&, const FBXM_ALLOCATOR<_Other>&)noexcept{
    // test for allocator inequality
    return (false);
}


inline void* operator new(std::size_t count){
    auto* ptr = FBXM_ALLOC(count);
    if(!ptr)
        throw std::bad_alloc{};
    return ptr;
}
inline void* operator new[](std::size_t count){
    auto* ptr = FBXM_ALLOC(count);
    if(!ptr)
        throw std::bad_alloc{};
    return ptr;
}
inline void* operator new(std::size_t count, const std::nothrow_t&)noexcept{
    return FBXM_ALLOC(count);
}
inline void* operator new[](std::size_t count, const std::nothrow_t&)noexcept{
    return FBXM_ALLOC(count);
}

inline void* operator new(std::size_t count, std::align_val_t al){
    auto* ptr = FBXM_ALIGN_ALLOC(count, al);
    if(!ptr)
        throw std::bad_alloc{};
    return ptr;
}
inline void* operator new[](std::size_t count, std::align_val_t al){
    auto* ptr = FBXM_ALIGN_ALLOC(count, al);
    if(!ptr)
        throw std::bad_alloc{};
    return ptr;
}
inline void* operator new(std::size_t count, std::align_val_t al, const std::nothrow_t&)noexcept{
    return FBXM_ALIGN_ALLOC(count, al);
}
inline void* operator new[](std::size_t count, std::align_val_t al, const std::nothrow_t&)noexcept{
    return FBXM_ALIGN_ALLOC(count, al);
}


inline void operator delete(void* ptr)noexcept{
    FBXM_FREE(ptr);
}
inline void operator delete[](void* ptr)noexcept{
    FBXM_FREE(ptr);
}
inline void operator delete(void* ptr, std::size_t sz)noexcept{
    FBXM_FREE(ptr);
}
inline void operator delete[](void* ptr, std::size_t sz)noexcept{
    FBXM_FREE(ptr);
}
inline void operator delete(void* ptr, const std::nothrow_t&)noexcept{
    FBXM_FREE(ptr);
}
inline void operator delete[](void* ptr, const std::nothrow_t&)noexcept{
    FBXM_FREE(ptr);
}

inline void operator delete(void* ptr, std::align_val_t al)noexcept{
    FBXM_ALIGN_FREE(ptr);
}
inline void operator delete[](void* ptr, std::align_val_t al)noexcept{
    FBXM_ALIGN_FREE(ptr);
}
inline void operator delete(void* ptr, std::size_t sz, std::align_val_t al)noexcept{
    FBXM_ALIGN_FREE(ptr);
}
inline void operator delete[](void* ptr, std::size_t sz, std::align_val_t al)noexcept{
    FBXM_ALIGN_FREE(ptr);
}
inline void operator delete(void* ptr, std::align_val_t al, const std::nothrow_t&)noexcept{
    FBXM_ALIGN_FREE(ptr);
}
inline void operator delete[](void* ptr, std::align_val_t al, const std::nothrow_t&)noexcept{
    FBXM_ALIGN_FREE(ptr);
}


template<
    class _Elem,
    class _Traits = std::char_traits<_Elem>
>
using fbx_basic_string = std::basic_string<_Elem, _Traits, FBXM_ALLOCATOR<_Elem>>;
using fbx_string = fbx_basic_string<FBX_CHAR>;

template<typename T>
using fbx_vector = std::vector<T, FBXM_ALLOCATOR<T>>;

template<typename T>
using fbx_list = std::list<T, FBXM_ALLOCATOR<T>>;

template<typename T>
using fbx_forward_list = std::forward_list<T, FBXM_ALLOCATOR<T>>;

template<typename T>
using fbx_deque = std::deque<T, FBXM_ALLOCATOR<T>>;

template<typename T>
using fbx_stack = std::stack<T, fbx_deque<T>>;

template<typename T>
using fbx_queue = std::queue<T, fbx_deque<T>>;

template<
    class _Kty,
    class _Pr = std::less<_Kty>
>
using fbx_set = std::set<_Kty, _Pr, FBXM_ALLOCATOR<_Kty>>;

template<
    class _Kty,
    class _Pr = std::less<_Kty>
>
using fbx_multiset = std::multiset<_Kty, _Pr, FBXM_ALLOCATOR<_Kty>>;

template<
    class _Kty,
    class _Ty,
    class _Pr = std::less<_Kty>
>
using fbx_map = std::map<_Kty, _Ty, _Pr, FBXM_ALLOCATOR<std::pair<const _Kty, _Ty>>>;

template<
    class _Kty,
    class _Ty,
    class _Pr = std::less<_Kty>
>
using fbx_multimap = std::multimap<_Kty, _Ty, _Pr, FBXM_ALLOCATOR<std::pair<const _Kty, _Ty>>>;

template<
    class _Kty,
    class _Hasher = std::hash<_Kty>,
    class _Keyeq = std::equal_to<_Kty>
>
using fbx_unordered_set = std::unordered_set<_Kty, _Hasher, _Keyeq, FBXM_ALLOCATOR<_Kty>>;

template<
    class _Kty,
    class _Hasher = std::hash<_Kty>,
    class _Keyeq = std::equal_to<_Kty>
>
using fbx_unordered_multiset = std::unordered_multiset<_Kty, _Hasher, _Keyeq, FBXM_ALLOCATOR<_Kty>>;

template<
    class _Kty,
    class _Ty,
    class _Hasher = std::hash<_Kty>,
    class _Keyeq = std::equal_to<_Kty>
>
using fbx_unordered_map = std::unordered_map<_Kty, _Ty, _Hasher, _Keyeq, FBXM_ALLOCATOR<std::pair<const _Kty, _Ty>>>;

template<
    class _Kty,
    class _Ty,
    class _Hasher = std::hash<_Kty>,
    class _Keyeq = std::equal_to<_Kty>
>
using fbx_unordered_multimap = std::unordered_multimap<_Kty, _Ty, _Hasher, _Keyeq, FBXM_ALLOCATOR<std::pair<const _Kty, _Ty>>>;


template<class _Ty>
inline fbx_basic_string<char> fbx_format_string(const char*_Fmt, _Ty _Val){
    const auto _Len = static_cast<size_t>(_scprintf(_Fmt, _Val));
    fbx_basic_string<char> _Str(_Len, '\0');
    sprintf_s(&_Str[0], _Len + 1, _Fmt, _Val);
    return (_Str);
}

template<class _Ty>
inline fbx_basic_string<wchar_t> fbx_format_wstring(const wchar_t*_Fmt, _Ty _Val){
    const auto _Len = static_cast<size_t>(_scwprintf(_Fmt, _Val));
    fbx_basic_string<wchar_t> _Str(_Len, L'\0');
    swprintf_s(&_Str[0], _Len + 1, _Fmt, _Val);
    return (_Str);
}

inline fbx_basic_string<char> fbx_to_string(int value){ return fbx_format_string("%d", value); }
inline fbx_basic_string<char> fbx_to_string(long value){ return fbx_format_string("%ld", value); }
inline fbx_basic_string<char> fbx_to_string(long long value){ return fbx_format_string("%lld", value); }
inline fbx_basic_string<char> fbx_to_string(unsigned value){ return fbx_format_string("%u", value); }
inline fbx_basic_string<char> fbx_to_string(unsigned long value){ return fbx_format_string("%lu", value); }
inline fbx_basic_string<char> fbx_to_string(unsigned long long value){ return fbx_format_string("%llu", value); }
inline fbx_basic_string<char> fbx_to_string(float value){ return fbx_format_string("%f", value); }
inline fbx_basic_string<char> fbx_to_string(double value){ return fbx_format_string("%f", value); }
inline fbx_basic_string<char> fbx_to_string(long double value){ return fbx_format_string("%Lf", value); }

inline fbx_basic_string<wchar_t> fbx_to_wstring(int value){ return fbx_format_wstring(L"%d", value); }
inline fbx_basic_string<wchar_t> fbx_to_wstring(long value){ return fbx_format_wstring(L"%ld", value); }
inline fbx_basic_string<wchar_t> fbx_to_wstring(long long value){ return fbx_format_wstring(L"%lld", value); }
inline fbx_basic_string<wchar_t> fbx_to_wstring(unsigned value){ return fbx_format_wstring(L"%u", value); }
inline fbx_basic_string<wchar_t> fbx_to_wstring(unsigned long value){ return fbx_format_wstring(L"%lu", value); }
inline fbx_basic_string<wchar_t> fbx_to_wstring(unsigned long long value){ return fbx_format_wstring(L"%llu", value); }
inline fbx_basic_string<wchar_t> fbx_to_wstring(float value){ return fbx_format_wstring(L"%f", value); }
inline fbx_basic_string<wchar_t> fbx_to_wstring(double value){ return fbx_format_wstring(L"%f", value); }
inline fbx_basic_string<wchar_t> fbx_to_wstring(long double value){ return fbx_format_wstring(L"%Lf", value); }
