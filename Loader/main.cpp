#include <cstdio>
#include <windows.h>

#include <string>
#include <unordered_map>

#include <FBXModule.hpp>


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
    library = LoadLibrary(TEXT("FBXModule.dll"));

    __FBXM_BIND_FUNC(library, FBXCheckCompatibility);

    __FBXM_BIND_FUNC(library, FBXGetErrorCount);
    __FBXM_BIND_FUNC(library, FBXGetLastError);

    __FBXM_BIND_FUNC(library, FBXOpenFile);
    __FBXM_BIND_FUNC(library, FBXCloseFile);

    __FBXM_BIND_FUNC(library, FBXReadScene);
    __FBXM_BIND_FUNC(library, FBXWriteScene);

    __FBXM_BIND_FUNC(library, FBXGetRoot);
    __FBXM_BIND_FUNC(library, FBXCopyRoot);
}
static inline void closeLib(){
    FreeLibrary(library);
}


static inline void loadFile(const char* name){
    if(!FBXOpenFile(name, "rb", (unsigned long)FBXIOType::FBXIOType_None, &setting)){
        printf_s("%s", getLastError().c_str());
        return;
    }
    
    if(!FBXReadScene()){
        printf_s("%s", getLastError().c_str());
        return;
    }

    {
        auto* pRootNode = reinterpret_cast<FBXRoot*>(FBXGetRoot());

        FBXAllocateRoot((void**)&fbxRoot, pRootNode);
        FBXCopyRoot(fbxRoot, pRootNode);
    }

    if(!FBXCloseFile()){
        printf_s("%s", getLastError().c_str());
        return;
    }
}

static inline void storeNode(const char* name){
    if(!FBXOpenFile(name, "wb", (unsigned long)FBXIOType::FBXIOType_None, &setting)){
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
