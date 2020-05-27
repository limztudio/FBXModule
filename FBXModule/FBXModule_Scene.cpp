/**
* @file FBXModule_Scene.cpp
* @date 2018/06/17
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <string>

#include <FBXModule.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


__FBXM_MAKE_FUNC(bool, FBXReadScene, void){
    static const char __name_of_this_func[] = "FBXReadScene(void)";


    if(!shr_root){
        SHRPushErrorMessage("this function is only available on read mode", __name_of_this_func);
        return false;
    }

    if(!shr_scene){
        SHRPushErrorMessage("scene must be opened before read", __name_of_this_func);
        return false;
    }

    {
        auto& kSceneGlobalSettings = shr_scene->GetGlobalSettings();

        { // axis conversion
            auto kSceneAxis = kSceneGlobalSettings.GetAxisSystem();
            if(kSceneAxis != shr_axisSystem)
                shr_axisSystem.ConvertScene(shr_scene);
        }

        { // unit conversion
            // this may cause memory leaks

            auto kSceneSystemUnit = kSceneGlobalSettings.GetSystemUnit();
            if(kSceneSystemUnit != shr_systemUnit)
                shr_systemUnit.ConvertScene(shr_scene);
        }
    }

    if(!SHRConvertOjbects(shr_SDKManager, shr_scene)){
        SHRPushErrorMessage("failed convert fbx objects", __name_of_this_func);
        return false;
    }

    {
        shr_materialTable.clear();
        shr_fbxNodeToExportNode.clear();

        if(!SHRGenerateNodeTree(shr_SDKManager, shr_scene, shr_materialTable, shr_fbxNodeToExportNode, &shr_root->Nodes)){
            SHRPushErrorMessage("an error occurred while generating object nodes", __name_of_this_func);
            return false;
        }

        if(!SHRLoadMaterials(shr_materialTable, &shr_root->Materials)){
            SHRPushErrorMessage("an error occurred while loading material data", __name_of_this_func);
            return false;
        }

        if(!SHRLoadAnimations(shr_SDKManager, shr_scene, shr_fbxNodeToExportNode, &shr_root->Animations)){
            SHRPushErrorMessage("an error occurred while loading animation data", __name_of_this_func);
            return false;
        }
    }

    return true;
}

__FBXM_MAKE_FUNC(bool, FBXWriteScene, const void* pRoot){
    static const char __name_of_this_func[] = "FBXWriteScene(const void*)";


    const auto* ext_root = reinterpret_cast<const FBXRoot*>(pRoot);

    {
        auto& kSceneGlobalSettings = shr_scene->GetGlobalSettings();

        { // axis conversion
            auto kSceneAxis = kSceneGlobalSettings.GetAxisSystem();
            if(kSceneAxis != shr_axisSystem)
                shr_axisSystem.ConvertScene(shr_scene);
        }

        { // unit conversion
            // this may cause memory leaks

            auto kSceneSystemUnit = kSceneGlobalSettings.GetSystemUnit();
            if(kSceneSystemUnit != shr_systemUnit)
                shr_systemUnit.ConvertScene(shr_scene);
        }
    }

    if(!shr_scene){
        SHRPushErrorMessage("scene must be created before write", __name_of_this_func);
        return false;
    }

    if(!SHRStoreMaterials(shr_SDKManager, shr_scene, ext_root->Materials)){
        SHRPushErrorMessage("an error occurred while storing materials", __name_of_this_func);
        return false;
    }

    if(!SHRStoreNodes(shr_SDKManager, shr_scene, shr_importNodeToFbxNode, shr_poseNodeList, ext_root->Nodes)){
        SHRPushErrorMessage("an error occurred while storing object nodes", __name_of_this_func);
        return false;
    }

    if(!SHRCreateBindPose(shr_SDKManager, shr_scene, shr_poseNodeList)){
        SHRPushErrorMessage("an error occurred while storing bind poses", __name_of_this_func);
        return false;
    }

    if(!SHRStoreAnimations(shr_SDKManager, shr_scene, shr_importNodeToFbxNode, ext_root->Animations)){
        SHRPushErrorMessage("an error occurred while storing animations", __name_of_this_func);
        return false;
    }

    return true;
}
