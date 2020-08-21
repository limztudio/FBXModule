/**
 * @file FBXUtilites.hpp
 * @date 2020/05/08
 * @author Lim Taewoo (limztudio@gmail.com)
 */


#ifndef _FBXUTILITES_HPP_
#define _FBXUTILITES_HPP_


#include "FBXType.hpp"


namespace __hidden_FBXModule{
    template<typename NODE, typename FUNC>
    static void _breakableIterateNode(bool& ret, NODE* pNode, FUNC func){
        if(ret)
            return;

        if(pNode){
            _breakableIterateNode(ret, pNode->Child, func);
            _breakableIterateNode(ret, pNode->Sibling, func);
            if(!func(pNode)){
                ret = true;
                return;
            }
        }
    }
};


/**
 * @brief Check if "target" contains "find".
 * @tparam Anytype which has bit operation.
 * @return Return true if "target" contains "find", and false otherwise.
 */
template<typename T>
static inline bool FBXTypeHasMember(T target, T find){
    const auto t = (unsigned long)target;
    const auto f = (unsigned long)find;

    return (t & f) == f;
}

/**
 * @brief Increase address of "p" till it has zero.
 * @return Difference between origin address and increased address.
 */
template<typename T>
static FBX_SIZE FBXGetMemoryLength(const T* p){
    FBX_SIZE i = 0;
    for(; (*p); ++p, ++i);
    return i;
}

/**
 * @brief Iterate selected node. Function will visit in postorder traversal(pretend left->child, right->sibling).
 * @param func Current node will be passed through function while iteration. Function must have "FBXNode*" as its parameter.
 */
template<typename NODE, typename FUNC>
static void FBXIterateNode(NODE* pNode, FUNC func){
    if(pNode){
        FBXIterateNode(pNode->Child, func);
        FBXIterateNode(pNode->Sibling, func);
        func(pNode);
    }
}
/**
 * @brief Iterate selected node. Function will visit in postorder traversal(pretend left->child, right->sibling).
 * @param func Current node will be passed through function while iteration. Function must have "FBXNode*" as its parameter. If function returns true, Iteration will be canceled.
 */
template<typename NODE, typename FUNC>
static void FBXBreakableIterateNode(NODE* pNode, FUNC func){
    bool ret = false;
    __hidden_FBXModule::_breakableIterateNode(ret, pNode, func);
}

/**
 * @brief Iterate parent node of selected node. Iteration will be continued till parent node set to nullptr.
 * @param func Current node will be passed through function while iteration. Function must have "FBXNode*" as its parameter.
 */
template<typename NODE, typename FUNC>
static void FBXIterateBackwardNode(NODE* pNode, FUNC func){
    if(pNode){
        for(; pNode; pNode = pNode->Parent)
            func(pNode);
    }
}
/**
 * @brief Iterate parent node of selected node. Iteration will be continued till parent node set to nullptr.
 * @param func Current node will be passed through function while iteration. Function must have "FBXNode*" as its parameter. If function returns true, Iteration will be canceled.
 */
template<typename NODE, typename FUNC>
static void FBXBreakableIterateBackwardNode(NODE* pNode, FUNC func){
    if(pNode){
        for(; pNode; pNode = pNode->Parent){
            if(!func(pNode))
                return;
        }
    }
}

/**
 * @brief Find first null object of current depth of "pNode".
 */
template<typename NODE>
static inline NODE*& FBXFindLastAddible(NODE*& pNode){
    if(!pNode)
        return pNode;

    auto** pTarget = &pNode->Sibling;
    for(; (*pTarget); pTarget = &(*pTarget)->Sibling);

    return *pTarget;
}


#endif // _FBXUTILITES_HPP_
