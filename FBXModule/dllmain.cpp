/**
 * @file dllmain.cpp
 * @date 2018/06/15
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#include "stdafx.h"

#include <cassert>

#include <FBXModule.hpp>

#include "FBXShared.h"


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved){
    switch(ul_reason_for_call){
    case DLL_PROCESS_ATTACH:
    case DLL_PROCESS_DETACH:
    //case DLL_THREAD_ATTACH:
    //case DLL_THREAD_DETACH:
        SHRDestroyFbxSdkObjects();
        SHRDeleteRoot();
        break;
    }

    return TRUE;
}
