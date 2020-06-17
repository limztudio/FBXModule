/**
* @file FBXModuleDecl.hpp
* @date 2020/05/27
* @author Lim Taewoo (limztudio@gmail.com)
*/


__FBXM_MAKE_FUNC(bool, FBXCheckCompatibility, void);

__FBXM_MAKE_FUNC(unsigned long, FBXGetErrorCount, void);
__FBXM_MAKE_FUNC(int, FBXGetLastError, TCHAR* szMessage);

__FBXM_MAKE_FUNC(unsigned long, FBXGetWarningCount, void);
__FBXM_MAKE_FUNC(int, FBXGetLastWarning, TCHAR* szMessage);

__FBXM_MAKE_FUNC(bool, FBXOpenFile, const TCHAR* szfilePath, const TCHAR* mode, const void* ioSetting);
__FBXM_MAKE_FUNC(bool, FBXCloseFile, void);

__FBXM_MAKE_FUNC(bool, FBXReadScene, void);
__FBXM_MAKE_FUNC(bool, FBXWriteScene, const void* pRoot);

__FBXM_MAKE_FUNC(const void*, FBXGetRoot, void);
__FBXM_MAKE_FUNC(void, FBXCopyRoot, void* pDest, const void* pSrc);

__FBXM_MAKE_FUNC(void, FBXGetWorldMatrix, void* pOutMatrix, const void* pNode);
__FBXM_MAKE_FUNC(void, FBXTransformCoord, void* pOutVec3, const void* pVec3, const void* pMatrix);
__FBXM_MAKE_FUNC(void, FBXTransformNormal, void* pOutVec3, const void* pVec3, const void* pMatrix);

__FBXM_MAKE_FUNC(void, FBXComputeAnimationTransform, void* pOutScale, void* pOutRotation, void* pOutTranslation, const void* pAnimationNode, float time);
