#include <tchar.h>

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include <filesystem>
#include <fstream>
#include <iostream>
#include <windows.h>

#include <string>

#define FBX_CHAR TCHAR
#include <FBXModule.hpp>
#include <FBXModuleDef.hpp>
#include <FBXModuleBind.hpp>


#ifdef _UNICODE
#define TSTRING wstring
#define TO_TSTRING std::to_wstring
#define TOFSTREAM std::wofstream
#define TOUT std::wcout
#else
#define TSTRING string
#define TO_TSTRING std::to_string
#define TOFSTREAM std::ofstream
#define TOUT std::cout
#endif


template<typename T>
struct fbx_remover{
    inline void operator()(T* p){ FBXDelete(p); }
};


static HMODULE library = nullptr;
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


static void exportOBJ(const FBXRoot* fbxRoot, std::basic_string<TCHAR>&& strOut){
    TOFSTREAM file(strOut);

    FBXIterateNode(fbxRoot->Nodes, [&file](const FBXNode* pNode){
        if(!FBXTypeHasMember(pNode->getID(), FBXType::FBXType_Mesh))
            return;

        const auto* pMesh = static_cast<const FBXMesh*>(pNode);

        for(size_t edxVert = pMesh->Vertices.Length, idxVert = 0; idxVert < edxVert; ++idxVert){
            const auto& iVert = pMesh->Vertices.Values[idxVert];
            
            file << "v ";
            file << iVert.Values[0] << " ";
            file << iVert.Values[1] << " ";
            file << iVert.Values[2] << "\n";
        }
        file << "\n";

        if(pMesh->LayeredElements.Length){
            const auto& cLayer = pMesh->LayeredElements.Values[0];

            for(size_t edxVert = cLayer.Texcoord.Length, idxVert = 0; idxVert < edxVert; ++idxVert){
                const auto& iVert = cLayer.Texcoord.Values[idxVert];

                file << "vt ";
                file << iVert.Values[0] << " ";
                file << iVert.Values[1] << "\n";
            }
            file << "\n";

            for(size_t edxVert = cLayer.Normal.Length, idxVert = 0; idxVert < edxVert; ++idxVert){
                const auto& iVert = cLayer.Normal.Values[idxVert];

                file << "vn ";
                file << iVert.Values[0] << " ";
                file << iVert.Values[1] << " ";
                file << iVert.Values[2] << "\n";
            }
            file << "\n";
        }

        for(size_t edxFace = pMesh->Indices.Length, idxFace = 0; idxFace < edxFace; ++idxFace){
            const auto& iFace = pMesh->Indices.Values[idxFace];

            file << "f ";
            file << iFace.Values[0] << " ";
            file << iFace.Values[1] << " ";
            file << iFace.Values[2] << "\n";
        }
        file << "\n";
    });

    for(size_t edxAnimStack = fbxRoot->Animations.Length, idxAnimStack = 0u; idxAnimStack < edxAnimStack; ++idxAnimStack){
        const auto& iAnimStack = fbxRoot->Animations.Values[idxAnimStack];

        for(size_t edxNode = iAnimStack.AnimationNodes.Length, idxNode = 0u; idxNode < edxNode; ++idxNode){
            const auto& iNode = iAnimStack.AnimationNodes.Values[idxNode];

            float fScale[3];
            float fRotation[4];
            float fTranslation[3];
            FBXComputeAnimationLocalTransform(fScale, fRotation, fTranslation, &iNode, iAnimStack.EndTime * 0.5f);

            file << "as: ";
            file << fScale[0] << " ";
            file << fScale[1] << " ";
            file << fScale[2] << "\n";

            file << "ar: ";
            file << fRotation[0] << " ";
            file << fRotation[1] << " ";
            file << fRotation[2] << " ";
            file << fRotation[3] << "\n";

            file << "at: ";
            file << fTranslation[0] << " ";
            file << fTranslation[1] << " ";
            file << fTranslation[2] << "\n";
        }
        file << "\n";
    }
    file << "\n";
}
static void exportFBX(const FBXRoot* fbxRoot, std::basic_string<TCHAR>&& strOut){
    if(!FBXOpenFile(strOut.c_str(), TEXT("wb"), &setting)){
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


static void task(const TCHAR* strIn, const TCHAR* strOut){
    static size_t _count = 0u;
    const std::basic_string<TCHAR> strCount(TO_TSTRING(_count++));

    if(!FBXOpenFile(strIn, TEXT("rb"), &setting)){
        TOUT << getLastError();
        return;
    }

    if(!FBXReadScene()){
        TOUT << getLastError();
        return;
    }

    const auto* pRoot = reinterpret_cast<const FBXRoot*>(FBXGetRoot());

    exportOBJ(pRoot, std::basic_string<TCHAR>(strOut) + strCount + TEXT(".obj"));

    std::unique_ptr<FBXRoot, fbx_remover<FBXRoot>> pCopiedRoot;
    {
        FBXRoot* ptr = nullptr;
        FBXCopyRoot(&ptr, pRoot);
        if(!ptr)
            return;
        pCopiedRoot.reset(ptr);
    }

    if(!FBXCloseFile()){
        TOUT << getLastError();
        return;
    }

    exportFBX(pCopiedRoot.get(), std::basic_string<TCHAR>(strOut) + strCount + TEXT(".fbx"));
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

    {
        std::filesystem::path rootPath(argv[1]);
        for(auto& p : std::filesystem::directory_iterator(rootPath)){
            auto curPath = p.path();
            auto curExt = curPath.extension().TSTRING();
            std::transform(curExt.begin(), curExt.end(), curExt.begin(), tolower);
            if(curExt != TEXT(".fbx"))
                continue;

            task(curPath.c_str(), argv[2]);
        }
    }

    //for(size_t i = 0; i < 1000; ++i)
    //task(argv[1], argv[2]);
    
    closeLib();

#ifdef _DEBUG
    {
        _CrtDumpMemoryLeaks();
    }
#endif

    return 0;
}
