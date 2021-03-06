﻿/**
 * @file FBXShared_Material.cpp
 * @date 2020/05/26
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#include "stdafx.h"

#include <FBXAssign.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


using namespace fbxsdk;


MaterialTable shr_materialTable;


bool SHRLoadMaterials(const MaterialTable& materialTable, FBXDynamicArray<FBXMaterial>* pMaterials){
    //static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRLoadMaterials(const MaterialTable&, FBXDynamicArray<FBXMaterial>*)");


    const auto& kMaterialTable = materialTable.getTable();
    auto& iMaterialTable = *pMaterials;

    iMaterialTable.Assign(kMaterialTable.size());

    for(size_t idxMaterial = 0u; idxMaterial < iMaterialTable.Length; ++idxMaterial){
        auto* kMaterial = kMaterialTable[idxMaterial];
        auto& iMaterial = iMaterialTable.Values[idxMaterial];

        CopyString(iMaterial.Name, ConvertString<FBX_CHAR>(kMaterial->GetName()));

        for(auto kProperty = kMaterial->GetFirstProperty(); kProperty.IsValid(); kProperty = kMaterial->GetNextProperty(kProperty)){
            int edxTex = kProperty.GetSrcObjectCount<FbxTexture>();
            for(auto idxTex = decltype(edxTex){ 0 }; idxTex < edxTex; ++idxTex){
                auto* kTexture = kProperty.GetSrcObject<FbxFileTexture>(idxTex);
                if(!kTexture)
                    continue;

                CopyString(iMaterial.DiffuseTexturePath, ConvertString<FBX_CHAR>(kTexture->GetFileName()));
                goto END_LOAD_MATERIAL;
            }
        }

    END_LOAD_MATERIAL:
        ;
    }

    return true;
}

bool SHRStoreMaterials(FbxManager* kSDKManager, FbxScene* kScene, const FBXDynamicArray<FBXMaterial>& materialTable){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRStoreMaterials(FbxManager*, FbxScene*, const FBXDynamicArray<FBXMaterial>&)");


    { // remove reserved materials
        const auto oldMatCount = kScene->GetMaterialCount();
        if(oldMatCount){
            fbx_unordered_set<FbxSurfaceMaterial*, PointerHasher<FbxSurfaceMaterial*>> oldMatTable;

            for(auto i = decltype(oldMatCount){ 0 }; i < oldMatCount; ++i){
                auto* kMaterial = kScene->GetMaterial(i);

                if(kMaterial)
                    oldMatTable.emplace(kMaterial);
            }

            for(auto* kMaterial : oldMatTable){
                const fbx_string strName = ConvertString<FBX_CHAR>(kMaterial->GetName());

                if(!kScene->RemoveMaterial(kMaterial)){
                    fbx_string msg = FBX_TEXT("failed to remove material from scene");
                    msg += FBX_TEXT("(errored in \"");
                    msg += strName;
                    msg += FBX_TEXT("\")");
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                    return false;
                }
            }
        }
    }

    for(size_t idxMaterial = 0u; idxMaterial < materialTable.Length; ++idxMaterial){
        const auto& iMaterial = materialTable.Values[idxMaterial];

        const fbx_string strName = iMaterial.Name.Values;

        auto* kMaterial = FbxSurfacePhong::Create(kSDKManager, ConvertString<char>(strName).c_str());
        if(!kMaterial){
            fbx_string msg = FBX_TEXT("failed to create FbxSurfacePhong");
            msg += FBX_TEXT("(errored in \"");
            msg += strName;
            msg += FBX_TEXT("\")");
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return false;
        }

        {
            kMaterial->Emissive.Set(FbxDouble3(0., 0., 0.));
            kMaterial->Ambient.Set(FbxDouble3(1., 1., 1.));
            kMaterial->Diffuse.Set(FbxDouble3(1., 1., 1.));
        }

        if(iMaterial.DiffuseTexturePath.Length){
            auto* kTexture = FbxFileTexture::Create(kScene, FbxSurfaceMaterial::sDiffuse);
            if(!kTexture){
                fbx_string msg = FBX_TEXT("failed to create FbxFileTexture");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            if(!kTexture->SetFileName(ConvertString<char>(iMaterial.DiffuseTexturePath.Values).c_str())){
                fbx_string msg = FBX_TEXT("set valid filename: \"");
                msg += iMaterial.DiffuseTexturePath.Values;
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            if(!kMaterial->Diffuse.ConnectSrcObject(kTexture)){
                fbx_string msg = FBX_TEXT("failed to connect diffuse texture object");
                msg += FBX_TEXT("(errored in \"");
                msg += strName;
                msg += FBX_TEXT("\")");
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }
        }

        if(!kScene->AddMaterial(kMaterial)){
            fbx_string msg = FBX_TEXT("failed to add material to scene");
            msg += FBX_TEXT("(errored in \"");
            msg += strName;
            msg += FBX_TEXT("\")");
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return false;
        }
    }

    return true;
}
