/**
* @file FBXShared_Material.cpp
* @date 2020/05/26
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <unordered_set>

#include <FBXAssign.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


using namespace fbxsdk;


MaterialTable shr_materialTable;


bool SHRLoadMaterials(const MaterialTable& materialTable, FBXDynamicArray<FBXMaterial>* pMaterials){
    static const char __name_of_this_func[] = "SHRLoadMaterials(const MaterialTable&, FBXDynamicArray<FBXMaterial>*)";


    const auto& kMaterialTable = materialTable.getTable();
    auto& iMaterialTable = *pMaterials;

    iMaterialTable.Assign(kMaterialTable.size());

    for(size_t idxMaterial = 0; idxMaterial < iMaterialTable.Length; ++idxMaterial){
        auto* kMaterial = kMaterialTable[idxMaterial];
        auto& iMaterial = iMaterialTable.Values[idxMaterial];

        CopyString(iMaterial.Name, kMaterial->GetName());

        { // diffuse
            auto kProperty = kMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);
            auto* kTexture = kProperty.GetSrcObject<FbxFileTexture>();
            if(kTexture){
                CopyString(iMaterial.DiffuseTexturePath, kTexture->GetFileName());
            }
        }
    }

    return true;
}

bool SHRStoreMaterials(FbxManager* kSDKManager, FbxScene* kScene, const FBXDynamicArray<FBXMaterial>& materialTable){
    static const char __name_of_this_func[] = "SHRStoreMaterials(FbxManager*, FbxScene*, const FBXDynamicArray<FBXMaterial>&)";


    { // remove reserved materials
        const auto oldMatCount = kScene->GetMaterialCount();
        if(oldMatCount){
            std::unordered_set<FbxSurfaceMaterial*, PointerHasher<FbxSurfaceMaterial*>> oldMatTable;

            for(auto i = decltype(oldMatCount){ 0 }; i < oldMatCount; ++i){
                auto* kMaterial = kScene->GetMaterial(i);

                if(kMaterial)
                    oldMatTable.emplace(kMaterial);
            }

            for(auto* kMaterial : oldMatTable){
                const std::string strName = kMaterial->GetName();

                if(!kScene->RemoveMaterial(kMaterial)){
                    std::string msg = "failed to remove material from scene";
                    msg += "(errored in \"";
                    msg += strName;
                    msg += "\")";
                    SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                    return false;
                }
            }
        }
    }

    for(size_t idxMaterial = 0; idxMaterial < materialTable.Length; ++idxMaterial){
        const auto& iMaterial = materialTable.Values[idxMaterial];

        const std::string strName = iMaterial.Name.Values;

        auto* kMaterial = FbxSurfacePhong::Create(kSDKManager, strName.c_str());
        if(!kMaterial){
            std::string msg = "failed to create FbxSurfacePhong";
            msg += "(errored in \"";
            msg += strName;
            msg += "\")";
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
                std::string msg = "failed to create FbxFileTexture";
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            if(!kTexture->SetFileName(iMaterial.DiffuseTexturePath.Values)){
                std::string msg = "set valid filename: \"";
                msg += iMaterial.DiffuseTexturePath.Values;
                msg += "\"";
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }

            if(!kMaterial->Diffuse.ConnectSrcObject(kTexture)){
                std::string msg = "failed to connect diffuse texture object";
                msg += "(errored in \"";
                msg += strName;
                msg += "\")";
                SHRPushErrorMessage(std::move(msg), __name_of_this_func);
                return false;
            }
        }

        if(!kScene->AddMaterial(kMaterial)){
            std::string msg = "failed to add material to scene";
            msg += "(errored in \"";
            msg += strName;
            msg += "\")";
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return false;
        }
    }

    return true;
}
