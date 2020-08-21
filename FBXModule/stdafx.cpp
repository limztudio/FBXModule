/**
 * @file stdafx.cpp
 * @date 2018/06/15
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#ifdef _WIN64
#ifdef _DEBUG
#pragma comment(lib, "fbxsdk/lib/x64/debug/libfbxsdk-md.lib")
#pragma comment(lib, "fbxsdk/lib/x64/debug/libxml2-md.lib")
#pragma comment(lib, "fbxsdk/lib/x64/debug/zlib-md.lib")
#else
#pragma comment(lib, "fbxsdk/lib/x64/release/libfbxsdk-md.lib")
#pragma comment(lib, "fbxsdk/lib/x64/release/libxml2-md.lib")
#pragma comment(lib, "fbxsdk/lib/x64/release/zlib-md.lib")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib, "fbxsdk/lib/x86/debug/libfbxsdk-md.lib")
#pragma comment(lib, "fbxsdk/lib/x86/debug/libxml2-md.lib")
#pragma comment(lib, "fbxsdk/lib/x86/debug/zlib-md.lib")
#else
#pragma comment(lib, "fbxsdk/lib/x86/release/libfbxsdk-md.lib")
#pragma comment(lib, "fbxsdk/lib/x86/release/libxml2-md.lib")
#pragma comment(lib, "fbxsdk/lib/x86/release/zlib-md.lib")
#endif
#endif


#ifdef _AFXDLL
#pragma comment(lib, "advapi32")
#endif


#include "stdafx.h"

#include "AllocateManager.hpp"
