#include <tchar.h>

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include <iostream>
#include <windows.h>

#include <string>

#define FBX_CHAR TCHAR
#include <FBXModule.hpp>
#include <FBXModuleDef.hpp>
#include <FBXModuleBind.hpp>


#ifdef _UNICODE
#define TOUT std::wcout
#else
#define TOUT std::cout
#endif


static HMODULE library = nullptr;
static FBXRoot* fbxRoot = nullptr;

static FBXIOSetting setting;


static inline std::basic_string<TCHAR> getLastError(){
    auto len = FBXGetLastError(nullptr);
    if(len > 0){
        std::basic_string<TCHAR> msg;
        msg.resize(len);
        FBXGetLastError(&msg[0]);
    }

    return TEXT("");
}


static inline void loadLib(){
#ifdef _DEBUG
    library = LoadLibrary(TEXT("FBXModuleD.dll"));
#else
    library = LoadLibrary(TEXT("FBXModule.dll"));
#endif

    FBXBindFunction(library);
}
static inline void closeLib(){
    if(!FreeLibrary(library))
        TOUT << GetLastError();
}


static inline void loadFile(const TCHAR* name){
    if(!FBXOpenFile(name, TEXT("rb"), &setting)){
        TOUT << getLastError();
        return;
    }
    
    if(!FBXReadScene()){
        TOUT << getLastError();
        return;
    }

    {
        const auto* pRootNode = reinterpret_cast<const FBXRoot*>(FBXGetRoot());

        FBXCopyRoot(&fbxRoot, pRootNode);
    }

    if(!FBXCloseFile()){
        TOUT << getLastError();
        return;
    }
}

static inline void storeNode(const TCHAR* name){
    if(!FBXOpenFile(name, TEXT("wb"), &setting)){
        TOUT << getLastError();
        return;
    }

    if(!FBXWriteScene(fbxRoot)){
        TOUT << getLastError();
        return;
    }

    if(!FBXCloseFile()){
        TOUT << getLastError();
        return;
    }
}

static inline void deleteFBXObjects(){
    if(fbxRoot)
        FBXDelete(fbxRoot);
}



int _tmain(int argc, TCHAR* argv[]){
#ifdef _DEBUG
    {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);

        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    }
#endif

    setting.AxisSystem = FBXAxisSystem::FBXAxisSystem_Preset_Max;

    loadLib();

    //for(size_t i = 0u; i < 10u; ++i)
    {
        loadFile(argv[1]);
        storeNode(argv[2]);
        deleteFBXObjects();
    }

    closeLib();

#ifdef _DEBUG
    {
        _CrtDumpMemoryLeaks();
    }
#endif

    return 0;
}
