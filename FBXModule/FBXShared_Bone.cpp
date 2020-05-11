/**
* @file FBXShared_Bone.cpp
* @date 2020/05/11
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <FBXAssign.hpp>

#include "FBXUtilites.h"
#include "FBXShared.h"


using namespace fbxsdk;


bool SHRInitBoneNode(FbxManager* kSDKManager, const FBXBone* pNode, FbxNode* kNode){
    auto* kSkeleton = kNode->GetSkeleton();

    return true;
}