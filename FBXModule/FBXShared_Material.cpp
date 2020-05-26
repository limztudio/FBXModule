/**
* @file FBXShared_Material.cpp
* @date 2020/05/26
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <FBXAssign.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


using namespace fbxsdk;


bool SHRLoadMaterial(MaterialElement& iMaterial, FbxSurfaceMaterial* kMaterial){
    static const char __name_of_this_func[] = "SHRLoadMaterial(MaterialElement&, FbxSurfaceMaterial*)";


    iMaterial.name = kMaterial->GetName();

    { // diffuse
        auto kProperty = kMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);
        auto* kTexture = kProperty.GetSrcObject<FbxFileTexture>();
        if(kTexture){
            iMaterial.diffusePath = kTexture->GetFileName();
        }
    }

    return true;
}
FbxSurfaceMaterial* SHRCreateMaterial(FbxManager* kSDKManager, FbxScene* kScene, const FBXMeshMaterial* pMaterial){
    static const char __name_of_this_func[] = "SHRCreateMaterial(FbxManager*, FbxScene*, const FBXMeshMaterial*)";


    const std::string strName = pMaterial->Name.Values;

    auto* kMaterial = FbxSurfacePhong::Create(kSDKManager, strName.c_str());
    if(!kMaterial){
        std::string msg = "failed to create FbxSurfacePhong";
        msg += "(errored in \"";
        msg += strName;
        msg += "\")";
        SHRPushErrorMessage(std::move(msg), __name_of_this_func);
        return nullptr;
    }

    {
        kMaterial->Emissive.Set(FbxDouble3(0., 0., 0.));
        kMaterial->Ambient.Set(FbxDouble3(1., 1., 1.));
        kMaterial->Diffuse.Set(FbxDouble3(1., 1., 1.));
    }

    if(pMaterial->DiffuseTexturePath.Length){
        auto* kTexture = FbxFileTexture::Create(kScene, FbxSurfaceMaterial::sDiffuse);
        if(!kTexture){
            std::string msg = "failed to create FbxFileTexture";
            msg += "(errored in \"";
            msg += strName;
            msg += "\")";
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return nullptr;
        }

        if(!kTexture->SetFileName(pMaterial->DiffuseTexturePath.Values)){
            std::string msg = "set valid filename: \"";
            msg += pMaterial->DiffuseTexturePath.Values;
            msg += "\"";
            msg += "(errored in \"";
            msg += strName;
            msg += "\")";
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return nullptr;
        }

        if(!kMaterial->Diffuse.ConnectSrcObject(kTexture)){
            std::string msg = "failed to connect diffuse texture object";
            msg += "(errored in \"";
            msg += strName;
            msg += "\")";
            SHRPushErrorMessage(std::move(msg), __name_of_this_func);
            return nullptr;
        }
    }

    return kMaterial;
}
