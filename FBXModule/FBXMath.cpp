/**
 * @file FBXMath.cpp
 * @date 2020/05/21
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#include "stdafx.h"

#include "FBXMath.h"


bool SIMDCompetible(){
    return DirectX::XMVerifyCPUSupport();
}
