/**
 * @file FBXModuleDecl.hpp
 * @date 2020/05/27
 * @author Lim Taewoo (limztudio@gmail.com)
 */


/**
 * @brief Check if current system supports FBXModule.
 * @return Return true if FBXModule is available, and false otherwise.
 */
__FBXM_MAKE_FUNC(bool, FBXCheckCompatibility, void);

/**
 * @brief Return total errored message count.
 * @return Total error count.
 */
__FBXM_MAKE_FUNC(unsigned long, FBXGetErrorCount, void);
/**
 * @brief Return last errored message.
 * @param szMessage String buffer to take message. If it set to nullptr, last message in the message stack will not be popped.
 * @return Length of last message.
 */
__FBXM_MAKE_FUNC(int, FBXGetLastError, FBX_CHAR* szMessage);

/**
 * @brief Return total warning message count.
 * @return Total warning count.
 */
__FBXM_MAKE_FUNC(unsigned long, FBXGetWarningCount, void);
/**
 * @brief Return last warning message.
 * @param szMessage String buffer to take message. If it set to nullptr, last message in the message stack will not be popped.
 * @return Length of last message.
 */
__FBXM_MAKE_FUNC(int, FBXGetLastWarning, FBX_CHAR* szMessage);

/**
 * @brief Estimate contained animations to reduce key frames.
 * @param szExcludeNames Bone name list which need to be excluded from reduction.
 * @param uExcludeNameCount Bone name list element count.
 * @param cMask A curve whose selected bit will be processed the reduction. (0x01 = translation; 0x02 = rotation; 0x04 = scaling)
 * @param fPrecision Default is 1.0
 * @return Return true if successfully reduced.
 */
__FBXM_MAKE_FUNC(bool, FBXReduceKeyframe, const FBX_CHAR** szExcludeNames, unsigned long uExcludeNameCount, unsigned char cMask, double fPrecision);

/**
 * @brief Open/Create selected FBX file.
 * @param szFilePath FBX file path.
 * @param mode "rb": Open file. "wb": Create file.
 * @param ioSetting Setting configuration. Must be passed by "const FBXIOSetting*".
 * @return Return true if successfully opened/created file, and false otherwise.
 */
__FBXM_MAKE_FUNC(bool, FBXOpenFile, const FBX_CHAR* szFilePath, const FBX_CHAR* mode, const void* ioSetting);
/**
 * @brief Close FBX file after open/create file.
 * @return Return true if successfully closed file, and false otherwise.
 */
__FBXM_MAKE_FUNC(bool, FBXCloseFile, void);

/**
 * @brief Read FBX scene.
 * @return Return true if successfully read scene, and false otherwise.
 */
__FBXM_MAKE_FUNC(bool, FBXReadScene, void);
/**
 * @brief Write FBX scene.
 * @param pRoot An object to be written on the FBX scene. Must be passed by "const FBXRoot*".
 * @return Return true if successfully written on the scene, and false otherwise.
 */
__FBXM_MAKE_FUNC(bool, FBXWriteScene, const void* pRoot);

/**
 * @brief Return current read scene.
 * @return Current scene.
 */
__FBXM_MAKE_FUNC(const void*, FBXGetRoot, void);

/**
 * @brief Return world matrix of selected node.
 * @param pOutMatrix Output world matrix. Must be set to an address of 16xfloat.
 * @param pNode Reference node. Must be passed by "const FBXNode*".
 */
__FBXM_MAKE_FUNC(void, FBXGetWorldMatrix, void* pOutMatrix, const void* pNode);
/**
 * @brief Transform vector3 unit by 4x4matrix.
 * @param pOutVec3 Output vector. Must be set to an address of 3xfloat.
 * @param pVec3 Input vector. Must be set to an address of 3xfloat.
 * @param pMatrix Transform matrix. Must be set to an address of 16xfloat.
 */
__FBXM_MAKE_FUNC(void, FBXTransformCoord, void* pOutVec3, const void* pVec3, const void* pMatrix);
/**
 * @brief Transform vector3 unit by 4x4matrix(without translation). Output vector will be normalized.
 * @param pOutVec3 Output vector. Must be set to an address of 3xfloat.
 * @param pVec3 Input vector. Must be set to an address of 3xfloat.
 * @param pMatrix Transform matrix. Must be set to an address of 16xfloat.
 */
__FBXM_MAKE_FUNC(void, FBXTransformNormal, void* pOutVec3, const void* pVec3, const void* pMatrix);

/**
 * @brief Compute local transform on specific time of animation node.
 * @param pOutScale Output scale of transform. Must be set to an address of 3xfloat.
 * @param pOutRotation Output quaternion(rotation) of transform. Must be set to an address of 4xfloat.
 * @param pOutTranslation Output translation of transform. Must be set to an address of 3xfloat.
 * @param pAnimationNode Reference animation node. Must be passed by "const FBXAnimationNode*".
 * @param time Time in second.
 */
__FBXM_MAKE_FUNC(void, FBXComputeAnimationLocalTransform, void* pOutScale, void* pOutRotation, void* pOutTranslation, const void* pAnimationNode, float time);
/**
 * @brief Compute world transform on specific time of animation node.
 * @param pOutScale Output scale of transform. Must be set to an address of 3xfloat.
 * @param pOutRotation Output quaternion(rotation) of transform. Must be set to an address of 4xfloat.
 * @param pOutTranslation Output translation of transform. Must be set to an address of 3xfloat.
 * @param pAnimationNode Reference animation node. Must be passed by "const FBXAnimationNode*".
 * @param time Time in second.
 */
__FBXM_MAKE_FUNC(void, FBXComputeAnimationWorldTransform, void* pOutScale, void* pOutRotation, void* pOutTranslation, const void* pAnimationNode, float time);

/**
 * @brief Compute local transform on specific time of animation node.
 * @param pOutScale Output scale of transform. Must be set to an address of 3xfloat.
 * @param pAnimationNode Reference animation node. Must be passed by "const FBXAnimationNode*".
 * @param time Time in second.
 */
__FBXM_MAKE_FUNC(void, FBXComputeAnimationLocalScale, void* pOutScale, const void* pAnimationNode, float time);
/**
 * @brief Compute world transform on specific time of animation node.
 * @param pOutScale Output scale of transform. Must be set to an address of 3xfloat.
 * @param pAnimationNode Reference animation node. Must be passed by "const FBXAnimationNode*".
 * @param time Time in second.
 */
__FBXM_MAKE_FUNC(void, FBXComputeAnimationWorldScale, void* pOutScale, const void* pAnimationNode, float time);

/**
 * @brief Compute local transform on specific time of animation node.
 * @param pOutRotation Output quaternion(rotation) of transform. Must be set to an address of 4xfloat.
 * @param pAnimationNode Reference animation node. Must be passed by "const FBXAnimationNode*".
 * @param time Time in second.
 */
__FBXM_MAKE_FUNC(void, FBXComputeAnimationLocalRotation, void* pOutRotation, const void* pAnimationNode, float time);
/**
 * @brief Compute world transform on specific time of animation node.
 * @param pOutRotation Output quaternion(rotation) of transform. Must be set to an address of 4xfloat.
 * @param pAnimationNode Reference animation node. Must be passed by "const FBXAnimationNode*".
 * @param time Time in second.
 */
__FBXM_MAKE_FUNC(void, FBXComputeAnimationWorldRotation, void* pOutRotation, const void* pAnimationNode, float time);

/**
 * @brief Compute local transform on specific time of animation node.
 * @param pOutTranslation Output translation of transform. Must be set to an address of 3xfloat.
 * @param pAnimationNode Reference animation node. Must be passed by "const FBXAnimationNode*".
 * @param time Time in second.
 */
__FBXM_MAKE_FUNC(void, FBXComputeAnimationLocalTranslation, void* pOutTranslation, const void* pAnimationNode, float time);
/**
 * @brief Compute world transform on specific time of animation node.
 * @param pOutTranslation Output translation of transform. Must be set to an address of 3xfloat.
 * @param pAnimationNode Reference animation node. Must be passed by "const FBXAnimationNode*".
 * @param time Time in second.
 */
__FBXM_MAKE_FUNC(void, FBXComputeAnimationWorldTranslation, void* pOutTranslation, const void* pAnimationNode, float time);


__FBXM_MAKE_HIDDEN_FUNC(void, 2555, __hidden_FBXModule_DeleteInnerObject, void* pObj);

__FBXM_MAKE_HIDDEN_FUNC(void, 2556, __hidden_FBXModule_RebindRoot, void* pDest, const void* pSrc);

__FBXM_MAKE_HIDDEN_FUNC(bool, 2557, __hidden_FBXModule_CollapseMesh, void** pDest, const void* pSrc, const void** pOldNodeList, const void** pNewerNodeList, unsigned long nodeCount);
