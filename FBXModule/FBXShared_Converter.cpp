/**
 * @file FBXShared_Converter.cpp
 * @date 2020/05/22
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#include "stdafx.h"

#include <FBXAssign.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


#define KEY_REDUCER_PRECISION 0.00001


using namespace fbxsdk;


bool SHRConvertNodes(FbxManager* kSDKManager, FbxNode* kNode){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRConvertNodes(FbxManager*, FbxNode*)");


    auto* kNodeAttribute = kNode->GetNodeAttribute();

    if(kNodeAttribute){
        switch(kNodeAttribute->GetAttributeType()){
        case FbxNodeAttribute::eMesh:
        {
            FbxGeometryConverter kConverter(kSDKManager);

            if(!kConverter.Triangulate(kNode->GetNodeAttribute(), true)){
                SHRPushErrorMessage(FBX_TEXT("failed to triangulate mesh object"), __name_of_this_func);
                return false;
            }

            break;
        }

        case FbxNodeAttribute::eNurbs:
        {
            FbxGeometryConverter kConverter(kSDKManager);

            if(!kConverter.Triangulate(kNode->GetNodeAttribute(), true)){
                SHRPushErrorMessage(FBX_TEXT("failed to triangulate nurbs object"), __name_of_this_func);
                return false;
            }
            //kConverter.TriangulateInPlace(kNode);

            break;
        }

        case FbxNodeAttribute::ePatch:
        {
            FbxGeometryConverter kConverter(kSDKManager);

            if(!kConverter.Triangulate(kNode->GetNodeAttribute(), true)){
                SHRPushErrorMessage(FBX_TEXT("failed to triangulate patch object"), __name_of_this_func);
                return false;
            }
            //kConverter.TriangulateInPlace(kNode);

            break;
        }

        default:
            break;
        }
    }

    for(int i = 0, e = kNode->GetChildCount(); i < e; ++i){
        if(!SHRConvertNodes(kSDKManager, kNode->GetChild(i)))
            return false;
    }

    return true;
}
bool SHRConvertAnimations(FbxManager* kSDKManager, FbxScene* kScene){
    //static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRConvertAnimations(FbxManager*, FbxScene*)");


    //FbxAnimCurveFilterKeyReducer kFilterReducer;
    //{
    //    kFilterReducer.SetPrecision(KEY_REDUCER_PRECISION);
    //}

    //FbxAnimCurveFilterConstantKeyReducer kFilterConstantReducer;
    //{
    //    kFilterConstantReducer.SetScalingThreshold(KEY_REDUCER_PRECISION);
    //    kFilterConstantReducer.SetRotationThreshold(KEY_REDUCER_PRECISION);
    //    kFilterConstantReducer.SetTranslationThreshold(KEY_REDUCER_PRECISION);
    //    kFilterConstantReducer.SetDefaultThreshold(KEY_REDUCER_PRECISION);

    //    kFilterConstantReducer.SetKeepFirstAndLastKeys(true);
    //}

    //for(auto edxAnimStack = kScene->GetSrcObjectCount<FbxAnimStack>(), idxAnimStack = 0; idxAnimStack < edxAnimStack; ++idxAnimStack){
    //    auto* kAnimStack = kScene->GetSrcObject<FbxAnimStack>(idxAnimStack);
    //    if(!kAnimStack)
    //        continue;

    //    if(!kFilterReducer.Apply(kAnimStack)){
    //        SHRPushErrorMessage(FBX_TEXT("failed to apply reduction filter to animation"), __name_of_this_func);
    //        return false;
    //    }

    //    if(!kFilterConstantReducer.Apply(kAnimStack)){
    //        SHRPushErrorMessage(FBX_TEXT("failed to apply constant reduction filter to animation"), __name_of_this_func);
    //        return false;
    //    }
    //}

    return true;
}
bool SHRPreparePointCaches(FbxScene* kScene){
    static const FBX_CHAR __name_of_this_func[] = FBX_TEXT("SHRPreparePointCaches(FbxScene*)");


    for(auto edxNode = kScene->GetNodeCount(), idxNode = 0; idxNode < edxNode; ++idxNode){
        auto* kNode = kScene->GetSrcObject<FbxNode>(idxNode);
        if(!kNode)
            continue;

        auto* kGeometry = kNode->GetGeometry();
        if(!kGeometry)
            continue;

        for(auto edxDeform = kGeometry->GetDeformerCount(FbxDeformer::eVertexCache), idxDeform = 0; idxDeform < edxDeform; ++idxDeform){
            auto* kDeform = static_cast<FbxVertexCacheDeformer*>(kGeometry->GetDeformer(idxDeform, FbxDeformer::eVertexCache));
            if(!kDeform)
                continue;

            auto* kCache = kDeform->GetCache();
            if(!kCache)
                continue;

            if(!kDeform->Active.Get())
                continue;

            if(kCache->GetCacheFileFormat() == FbxCache::eMaxPointCacheV2){
                // This code show how to convert from PC2 to MC point cache format
                // turn it on if you need it.
            }
            else if(kCache->GetCacheFileFormat() == FbxCache::eMayaCache){
                // This code show how to convert from MC to PC2 point cache format
                // turn it on if you need it.
                //
                if(!kCache->ConvertFromMCToPC2(FbxTime::GetFrameRate(kScene->GetGlobalSettings().GetTimeMode()), 0)){
                    SHRPushErrorMessage(FBX_TEXT("failed to ConvertFromMCToPC2"), __name_of_this_func);
                    return false;

                    // Conversion failed, retrieve the error here
                    //FbxString kTheErrorIs = kCache->GetError().GetLastErrorString();
                }
            }

            if(kCache->OpenFileForRead()){
                kDeform->Active.Set(false);
            }
        }
    }

    return true;
}

bool SHRConvertOjbects(FbxManager* kSDKManager, FbxScene* kScene){
    if(!SHRConvertNodes(kSDKManager, kScene->GetRootNode()))
        return false;

    if(!shr_ioSetting.IgnoreAnimationIO){
        if(!SHRConvertAnimations(kSDKManager, kScene))
            return false;
    }

    if(!SHRPreparePointCaches(kScene))
        return false;

    return true;
}
