/**
* @file FBXUtilites_Hierarchy.cpp
* @date 2018/07/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include "FBXUtilites.h"


using namespace fbxsdk;


static void ins_convertObjectRecursive(FbxManager* kSDKManager, FbxNode* kNode){
    auto* kNodeAttribute = kNode->GetNodeAttribute();

    if(kNodeAttribute){
        switch(kNodeAttribute->GetAttributeType()){
        case FbxNodeAttribute::eMesh:
        {
            FbxGeometryConverter kConverter(kSDKManager);

            kConverter.Triangulate(kNode->GetNodeAttribute(), true);
        }
        break;

        case FbxNodeAttribute::eNurbs:
        case FbxNodeAttribute::ePatch:
        {
            FbxGeometryConverter kConverter(kSDKManager);

            kConverter.Triangulate(kNode->GetNodeAttribute(), true);
            //kConverter.TriangulateInPlace(kNode);
        }
        break;
        }
    }

    for(int i = 0, e = kNode->GetChildCount(); i < e; ++i)
        ins_convertObjectRecursive(kSDKManager, kNode->GetChild(i));
}


void ConvertObjects(FbxManager* kSDKManager, FbxScene* kScene){
    ins_convertObjectRecursive(kSDKManager, kScene->GetRootNode());
}
