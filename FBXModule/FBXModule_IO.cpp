/**
* @file FBXModule_IO.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <filesystem>
#include <string>

#include <FBXModule.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


#ifdef _UNICODE
#define __tstring wstring
#else
#define __tstring string
#endif


using namespace fbxsdk;


class _DirectoryModifier{
public:
    _DirectoryModifier(const std::basic_string<FBX_CHAR>& changeDir)
        :
        orgDir(GetCurrentDirectory(0, nullptr), 0)
    {
        GetCurrentDirectory(orgDir.length(), orgDir.data());
        SetCurrentDirectory(changeDir.c_str());
    }
    ~_DirectoryModifier(){
        SetCurrentDirectory(orgDir.c_str());
    }


private:
    std::basic_string<FBX_CHAR> orgDir;
};


static FbxIOSettings* ins_IOSettings = nullptr;

static std::filesystem::path ins_filePath;
static unsigned char ins_fileMode = 0;


__FBXM_MAKE_FUNC(bool, FBXOpenFile, const FBX_CHAR* szFilePath, const FBX_CHAR* mode, const void* ioSetting){
    static const FBX_CHAR __name_of_this_func[] = TEXT("FBXOpenFile(const char*, const char*, unsigned long, const void*)");


    if(ioSetting)
        shr_ioSetting = (*reinterpret_cast<const FBXIOSetting*>(ioSetting));

    {
        if(shr_ioSetting.MaxBoneCountPerMesh < shr_ioSetting.MaxParticipateClusterPerVertex){
            SHRPushErrorMessage(TEXT("\'MaxBoneCountPerMesh\' must be bigger or equal to \'MaxParticipateClusterPerVertex\'"), __name_of_this_func);
            return false;
        }

        { // axis converter & unit setting
            FbxAxisSystem::EUpVector kUpVector = FbxAxisSystem::eYAxis;
            switch(FBXAxisSystem((unsigned long)shr_ioSetting.AxisSystem & (unsigned long)FBXAxisSystem::FBXAxisSystem_UpVector_Mask)){
            case FBXAxisSystem::FBXAxisSystem_UpVector_XAxis:
                kUpVector = FbxAxisSystem::eXAxis;
                break;
            case FBXAxisSystem::FBXAxisSystem_UpVector_NegXAxis:
                kUpVector = decltype(kUpVector)(-FbxAxisSystem::eXAxis);
                break;
            case FBXAxisSystem::FBXAxisSystem_UpVector_YAxis:
                kUpVector = FbxAxisSystem::eYAxis;
                break;
            case FBXAxisSystem::FBXAxisSystem_UpVector_NegYAxis:
                kUpVector = decltype(kUpVector)(-FbxAxisSystem::eYAxis);
                break;
            case FBXAxisSystem::FBXAxisSystem_UpVector_ZAxis:
                kUpVector = FbxAxisSystem::eZAxis;
                break;
            case FBXAxisSystem::FBXAxisSystem_UpVector_NegZAxis:
                kUpVector = decltype(kUpVector)(-FbxAxisSystem::eZAxis);
                break;
            default:
                SHRPushErrorMessage(TEXT("UpVector in \'AxisSystem\' has invalid value"), __name_of_this_func);
                return false;
            }

            FbxAxisSystem::EFrontVector kFrontVector = FbxAxisSystem::eParityOdd;
            switch(FBXAxisSystem((unsigned long)shr_ioSetting.AxisSystem & (unsigned long)FBXAxisSystem::FBXAxisSystem_FrontVector_Mask)){
            case FBXAxisSystem::FBXAxisSystem_FrontVector_ParityEven:
                kFrontVector = FbxAxisSystem::eParityEven;
                break;
            case FBXAxisSystem::FBXAxisSystem_FrontVector_NegParityEven:
                kFrontVector = decltype(kFrontVector)(-FbxAxisSystem::eParityEven);
                break;
            case FBXAxisSystem::FBXAxisSystem_FrontVector_ParityOdd:
                kFrontVector = FbxAxisSystem::eParityOdd;
                break;
            case FBXAxisSystem::FBXAxisSystem_FrontVector_NegParityOdd:
                kFrontVector = decltype(kFrontVector)(-FbxAxisSystem::eParityOdd);
                break;
            default:
                SHRPushErrorMessage(TEXT("FrontVector in \'AxisSystem\' has invalid value"), __name_of_this_func);
                return false;
            }

            FbxAxisSystem::ECoordSystem kCoordSystem = FbxAxisSystem::eLeftHanded;
            switch(FBXAxisSystem((unsigned long)shr_ioSetting.AxisSystem & (unsigned long)FBXAxisSystem::FBXAxisSystem_CoordSystem_Mask)){
            case FBXAxisSystem::FBXAxisSystem_CoordSystem_LeftHanded:
                kCoordSystem = FbxAxisSystem::eLeftHanded;
                break;
            case FBXAxisSystem::FBXAxisSystem_CoordSystem_RightHanded:
                kCoordSystem = FbxAxisSystem::eRightHanded;
                break;
            default:
                SHRPushErrorMessage(TEXT("CoordSystem in \'AxisSystem\' has invalid value"), __name_of_this_func);
                return false;
            }

            shr_axisSystem = FbxAxisSystem(kUpVector, kFrontVector, kCoordSystem);
            shr_systemUnit = FbxSystemUnit(shr_ioSetting.UnitScale, shr_ioSetting.UnitMultiplier);
        }
    }

    if((*mode == L'r') || (*mode == L'R')){
        ins_fileMode = 1;
        ins_filePath = szFilePath;
    }
    else if((*mode == L'w') || (*mode == L'W')){
        ins_fileMode = 2;
        ins_filePath = szFilePath;
    }
    else{
        ins_fileMode = 0;
        ins_filePath.clear();

        SHRPushErrorMessage(TEXT("unexpected file mode"), __name_of_this_func);
        return false;
    }

    if(shr_SDKManager){
        SHRPushErrorMessage(TEXT("close current file before open a new one"), __name_of_this_func);
        return false;
    }

    shr_SDKManager = FbxManager::Create();
    if(!shr_SDKManager){
        SHRPushErrorMessage(TEXT("null returned from FbxManager::Create(...)"), __name_of_this_func);
        return false;
    }

    ins_IOSettings = FbxIOSettings::Create(shr_SDKManager, IOSROOT);
    if(!ins_IOSettings){
        SHRPushErrorMessage(TEXT("null returned from FbxIOSettings::Create(...)"), __name_of_this_func);
        return false;
    }

    shr_SDKManager->SetIOSettings(ins_IOSettings);

    shr_scene = FbxScene::Create(shr_SDKManager, "");
    if(!shr_scene){
        SHRPushErrorMessage(TEXT("null returned from FbxScene::Create(...)"), __name_of_this_func);
        return false;
    }

    shr_scene->GetGlobalSettings().SetTimeMode(FbxTime::eFrames30);

    if(ins_fileMode == 1){
        if(!std::filesystem::exists(ins_filePath)){
            std::basic_string<FBX_CHAR> msg = TEXT('\"') + ins_filePath.__tstring();
            msg += TEXT("\" file not exist");
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return false;
        }
        if(std::filesystem::is_directory(ins_filePath)){
            std::basic_string<FBX_CHAR> msg = TEXT('\"') + ins_filePath.__tstring();
            msg += TEXT("\" must be a file not directory");
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return false;
        }

        _DirectoryModifier dirMod(ins_filePath.parent_path().__tstring());

        auto* kImporter = FbxImporter::Create(shr_SDKManager, "");
        if(!kImporter){
            SHRPushErrorMessage(TEXT("null returned from FbxImporter::Create(...)"), __name_of_this_func);
            return false;
        }

        CustomStream stream(shr_SDKManager, ins_filePath.__tstring(), TEXT("rb"));

        void* streamData = nullptr;
        if(!kImporter->Initialize(&stream, streamData, -1, ins_IOSettings)){
            SHRPushErrorMessage(TEXT("an error occurred from FbxImporter::Initialize(...)"), __name_of_this_func);
            return false;
        }

        if(kImporter->IsFBX()){
            //ins_IOSettings->SetBoolProp(IMP_FBX_CONSTRAINT, true);
            //ins_IOSettings->SetBoolProp(IMP_FBX_CONSTRAINT_COUNT, true);
            ins_IOSettings->SetBoolProp(IMP_FBX_MATERIAL, true);
            ins_IOSettings->SetBoolProp(IMP_FBX_TEXTURE, true);
            ins_IOSettings->SetBoolProp(IMP_FBX_LINK, true);
            ins_IOSettings->SetBoolProp(IMP_FBX_SHAPE, true);
            ins_IOSettings->SetBoolProp(IMP_FBX_GOBO, false);
            ins_IOSettings->SetBoolProp(IMP_FBX_ANIMATION, true);
            ins_IOSettings->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
        }

        if(!kImporter->Import(shr_scene)){
            SHRPushErrorMessage(TEXT("an error occurred from FbxImporter::Import(...)"), __name_of_this_func);
            return false;
        }

        kImporter->Destroy();

        SHRCreateRoot();
    }

    return true;
}
__FBXM_MAKE_FUNC(bool, FBXCloseFile, void){
    static const FBX_CHAR __name_of_this_func[] = TEXT("FBXCloseFile(void)");


    if((!shr_SDKManager) || (!ins_IOSettings)){
        SHRPushErrorMessage(TEXT("file must be opened/created before close"), __name_of_this_func);
        return false;
    }

    if(ins_fileMode == 1){
        SHRDeleteRoot();
    }
    else if(ins_fileMode == 2){
        if(ins_filePath.empty()){
            SHRPushErrorMessage(TEXT("file name must be set before export"), __name_of_this_func);
            return false;
        }
        if(std::filesystem::is_directory(ins_filePath)){
            std::basic_string<FBX_CHAR> msg = TEXT('\"') + ins_filePath.__tstring();
            msg += TEXT("\" must be a file path");
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return false;
        }

        const int writeFormat = shr_ioSetting.ExportAsASCII ? (-1) : shr_SDKManager->GetIOPluginRegistry()->GetNativeWriterFormat();
        {
            //ins_IOSettings->SetBoolProp(EXP_FBX_CONSTRAINT, true);
            ins_IOSettings->SetBoolProp(EXP_FBX_MATERIAL, true);
            ins_IOSettings->SetBoolProp(EXP_FBX_TEXTURE, true);
            ins_IOSettings->SetBoolProp(EXP_FBX_EMBEDDED, writeFormat != -1);
            ins_IOSettings->SetBoolProp(EXP_FBX_SHAPE, true);
            ins_IOSettings->SetBoolProp(EXP_FBX_GOBO, false);
            ins_IOSettings->SetBoolProp(EXP_FBX_ANIMATION, true);
            ins_IOSettings->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);
        }

        _DirectoryModifier dirMod(ins_filePath.parent_path().__tstring());

        auto* kExporter = FbxExporter::Create(shr_SDKManager, "");
        if(!kExporter){
            SHRPushErrorMessage(TEXT("null returned from FbxExporter::Create(...)"), __name_of_this_func);
            return false;
        }

        CustomStream stream(shr_SDKManager, ins_filePath.__tstring(), TEXT("wb"), shr_ioSetting.ExportAsASCII);

        void* streamData = nullptr;
        if(!kExporter->Initialize(&stream, streamData, writeFormat, ins_IOSettings)){
            SHRPushErrorMessage(TEXT("an error occurred from FbxExporter::Initialize(...)"), __name_of_this_func);
            return false;
        }

        if(!kExporter->Export(shr_scene)){
            SHRPushErrorMessage(TEXT("an error occurred from FbxExporter::Export(...)"), __name_of_this_func);
            return false;
        }
        kExporter->Destroy();
    }

    SHRDestroyFbxSdkObjects();

    ins_IOSettings = nullptr;

    return true;
}
