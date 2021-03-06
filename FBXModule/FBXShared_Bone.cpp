﻿/**
 * @file FBXShared_Bone.cpp
 * @date 2020/05/11
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#include "stdafx.h"

#include <FBXAssign.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


using namespace fbxsdk;


bool SHRLoadBoneNode(FbxManager* kSDKManager, const FbxNode* kNode, FBXBone* pNode){
    auto* kBone = (FbxSkeleton*)kNode->GetNodeAttribute();
    if(kBone){
        pNode->Size = decltype(pNode->Size)(kBone->Size);
        pNode->Length = decltype(pNode->Length)(kBone->LimbLength);
    }

    return true;
}

bool SHRInitBoneNode(FbxManager* kSDKManager, const FBXBone* pNode, FbxNode* kNode){
    auto* kBone = (FbxSkeleton*)kNode->GetNodeAttribute();
    if(kBone){
        kBone->SetSkeletonType(FbxSkeleton::eLimbNode);
        kBone->Size = pNode->Size;
        kBone->LimbLength = pNode->Length;
    }

    return true;
}
