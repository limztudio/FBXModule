#include <cstdio>
#include <windows.h>

#include <string>
#include <unordered_map>

#include <FBXModule.hpp>
#include <FBXModuleDef.hpp>
#include <FBXModuleBind.hpp>


static HMODULE library = nullptr;
static FBXRoot* fbxRoot = nullptr;

static FBXIOSetting setting;


static inline std::string getLastError(){
    auto len = FBXGetLastError(nullptr);
    if(len > 0){
        std::string msg;
        msg.resize(len);
        FBXGetLastError(&msg[0]);
    }

    return "";
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


static inline void loadFile(const char* name){
    if(!FBXOpenFile(name, "rb", &setting)){
        printf_s("%s", getLastError().c_str());
        return;
    }
    
    if(!FBXReadScene()){
        printf_s("%s", getLastError().c_str());
        return;
    }

    {
        const auto* pRootNode = reinterpret_cast<const FBXRoot*>(FBXGetRoot());

        FBXAllocateRoot(&fbxRoot, pRootNode);
        FBXCopyRoot(fbxRoot, pRootNode);
    }

    if(!FBXCloseFile()){
        printf_s("%s", getLastError().c_str());
        return;
    }
}

static inline void storeNode(const char* name){
    if(!FBXOpenFile(name, "wb", &setting)){
        printf_s("%s", getLastError().c_str());
        return;
    }

    if(!FBXWriteScene(fbxRoot)){
        printf_s("%s", getLastError().c_str());
        return;
    }

    if(!FBXCloseFile()){
        printf_s("%s", getLastError().c_str());
        return;
    }
}

static inline void deleteFBXObjects(){
    if(fbxRoot)
        FBXDelete(fbxRoot);
}



int main(int argc, char* argv[]){
    setting.AxisSystem = FBXAxisSystem::FBXAxisSystem_Preset_Max;

    loadLib();

    loadFile(argv[1]);
    storeNode(argv[2]);

    deleteFBXObjects();
    closeLib();
    return 0;
}
