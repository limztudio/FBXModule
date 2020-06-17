#include <tchar.h>

#include <iostream>
#include <windows.h>

#include <string>
#include <unordered_map>

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
    FreeLibrary(library);
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

        FBXAllocateRoot(&fbxRoot, pRootNode);
        FBXCopyRoot(fbxRoot, pRootNode);
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
    setting.AxisSystem = FBXAxisSystem::FBXAxisSystem_Preset_Max;

    loadLib();

    loadFile(argv[1]);
    storeNode(argv[2]);

    deleteFBXObjects();
    closeLib();
    return 0;
}
