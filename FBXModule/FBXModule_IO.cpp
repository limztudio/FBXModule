/**
* @file FBXModule_IO.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <eastl/string.h>

#include <FBXModule.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


using namespace fbxsdk;


static FbxIOSettings* ins_IOSettings = nullptr;

static eastl::string ins_fileName;
static unsigned char ins_fileMode = 0;


__FBXM_MAKE_FUNC(bool, FBXOpenFile, const char* szfilePath, const char* mode){
    static const char __name_of_this_func[] = "FBXOpenFile(const char*, const char*)";


    if((*mode == 'r') || (*mode == 'R')){
        ins_fileMode = 1;
        ins_fileName = szfilePath;
    }
    else if((*mode == 'w') || (*mode == 'W')){
        ins_fileMode = 2;
        ins_fileName = szfilePath;
    }
    else{
        ins_fileMode = 0;
        ins_fileName.clear();

        SHRPushErrorMessage("unexpected file mode", __name_of_this_func);
        return false;
    }

    if(shr_SDKManager){
        SHRPushErrorMessage("close current file before open a new one", __name_of_this_func);
        return false;
    }

    shr_SDKManager = FbxManager::Create();
    if(!shr_SDKManager){
        SHRPushErrorMessage("null returned from FbxManager::Create(...)", __name_of_this_func);
        return false;
    }

    ins_IOSettings = FbxIOSettings::Create(shr_SDKManager, IOSROOT);
    if(!ins_IOSettings){
        SHRPushErrorMessage("null returned from FbxIOSettings::Create(...)", __name_of_this_func);
        return false;
    }

    shr_SDKManager->SetIOSettings(ins_IOSettings);

    shr_scene = FbxScene::Create(shr_SDKManager, "");
    if(!shr_scene){
        SHRPushErrorMessage("null returned from FbxScene::Create(...)", __name_of_this_func);
        return false;
    }

    shr_scene->GetGlobalSettings().SetTimeMode(FbxTime::eFrames30);

    if(ins_fileMode == 1){
        auto* kImporter = FbxImporter::Create(shr_SDKManager, "");
        if(!kImporter){
            SHRPushErrorMessage("null returned from FbxImporter::Create(...)", __name_of_this_func);
            return false;
        }

        CustomStream stream(shr_SDKManager, szfilePath, "rb");

        void* streamData = nullptr;
        if(!kImporter->Initialize(&stream, streamData, -1, ins_IOSettings)){
            SHRPushErrorMessage("an error occurred from FbxImporter::Initialize(...)", __name_of_this_func);
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
            SHRPushErrorMessage("an error occurred from FbxImporter::Import(...)", __name_of_this_func);
            return false;
        }

        kImporter->Destroy();
    }

    return true;
}
__FBXM_MAKE_FUNC(bool, FBXCloseFile, void){
    static const char __name_of_this_func[] = "FBXCloseFile(void)";


    if((!shr_SDKManager) || (!ins_IOSettings)){
        SHRPushErrorMessage("file must be opened/created before close", __name_of_this_func);
        return false;
    }

    if(ins_fileMode == 2){
        if(ins_fileName.empty()){
            SHRPushErrorMessage("file name must be set before export", __name_of_this_func);
            return false;
        }

        {
            //ins_IOSettings->SetBoolProp(EXP_FBX_CONSTRAINT, true);
            ins_IOSettings->SetBoolProp(EXP_FBX_MATERIAL, true);
            ins_IOSettings->SetBoolProp(EXP_FBX_TEXTURE, true);
            ins_IOSettings->SetBoolProp(EXP_FBX_EMBEDDED, false);
            ins_IOSettings->SetBoolProp(EXP_FBX_SHAPE, true);
            ins_IOSettings->SetBoolProp(EXP_FBX_GOBO, false);
            ins_IOSettings->SetBoolProp(EXP_FBX_ANIMATION, true);
            ins_IOSettings->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);
        }

        auto* kExporter = FbxExporter::Create(shr_SDKManager, "");
        if(!kExporter){
            SHRPushErrorMessage("null returned from FbxExporter::Create(...)", __name_of_this_func);
            return false;
        }

        CustomStream stream(shr_SDKManager, ins_fileName.c_str(), "wb");

        void* streamData = nullptr;
        if(!kExporter->Initialize(&stream, streamData, -1, ins_IOSettings)){
            SHRPushErrorMessage("an error occurred from FbxExporter::Initialize(...)", __name_of_this_func);
            return false;
        }

        if(!kExporter->Export(shr_scene)){
            SHRPushErrorMessage("an error occurred from FbxExporter::Export(...)", __name_of_this_func);
            return false;
        }
        kExporter->Destroy();
    }

    SHRDestroyFbxSdkObjects();
    SHRDeleteAllNodes();
    SHRDeleteAllAnimations();

    ins_IOSettings = nullptr;

    return true;
}