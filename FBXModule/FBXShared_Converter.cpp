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
    static const char __name_of_this_func[] = "SHRConvertNodes(FbxManager*, FbxNode*)";


    auto* kNodeAttribute = kNode->GetNodeAttribute();

    if(kNodeAttribute){
        switch(kNodeAttribute->GetAttributeType()){
        case FbxNodeAttribute::eMesh:
        {
            FbxGeometryConverter kConverter(kSDKManager);

            if(!kConverter.Triangulate(kNode->GetNodeAttribute(), true)){
                SHRPushErrorMessage("failed to triangulate mesh object", __name_of_this_func);
                return false;
            }
        }
        break;

        case FbxNodeAttribute::eNurbs:
        {
            FbxGeometryConverter kConverter(kSDKManager);

            if(!kConverter.Triangulate(kNode->GetNodeAttribute(), true)){
                SHRPushErrorMessage("failed to triangulate nurbs object", __name_of_this_func);
                return false;
            }
            //kConverter.TriangulateInPlace(kNode);
        }
        break;

        case FbxNodeAttribute::ePatch:
        {
            FbxGeometryConverter kConverter(kSDKManager);

            if(!kConverter.Triangulate(kNode->GetNodeAttribute(), true)){
                SHRPushErrorMessage("failed to triangulate patch object", __name_of_this_func);
                return false;
            }
            //kConverter.TriangulateInPlace(kNode);
        }
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
    static const char __name_of_this_func[] = "SHRConvertAnimations(FbxManager*, FbxScene*)";


    for(auto edxAnimStack = kScene->GetSrcObjectCount<FbxAnimStack>(), idxAnimStack = 0; idxAnimStack < edxAnimStack; ++idxAnimStack){
        auto* kAnimStack = kScene->GetSrcObject<FbxAnimStack>(idxAnimStack);
        if(!kAnimStack)
            continue;

        //{
        //    FbxAnimCurveFilterKeyReducer kFilter;
        //    kFilter.SetPrecision(KEY_REDUCER_PRECISION);
        //    if(!kFilter.Apply(kAnimStack)){
        //        SHRPushErrorMessage("failed to apply reduction filter to animation", __name_of_this_func);
        //        return false;
        //    }
        //}

        //{
        //    FbxAnimCurveFilterConstantKeyReducer kFilter;
        //    if(!kFilter.Apply(kAnimStack)){
        //        SHRPushErrorMessage("failed to apply constant reduction filter to animation", __name_of_this_func);
        //        return false;
        //    }
        //}
    }

    return true;
}

bool SHRConvertOjbects(FbxManager* kSDKManager, FbxScene* kScene){
    if(!SHRConvertNodes(kSDKManager, kScene->GetRootNode()))
        return false;
    if(!SHRConvertAnimations(kSDKManager, kScene))
        return false;

    return true;
}
