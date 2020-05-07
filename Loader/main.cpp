#include <cstdio>
#include <windows.h>

#include <eastl/string.h>
#include <eastl/unordered_map.h>

#include <FBXModule.hpp>

#define _XM_SSE4_INTRINSICS_
#define _XM_AVX_INTRINSICS_
#define _XM_FMA3_INTRINSICS_
#include "DirectXMath/Inc/DirectXMath.h"
#include "DirectXMath/Extensions/DirectXMathSSE4.h"
#include "DirectXMath/Extensions/DirectXMathAVX.h"
#include "DirectXMath/Extensions/DirectXMathFMA3.h"


static HMODULE library = nullptr;


static inline eastl::string getLastError(){
    auto len = FBXGetLastError(nullptr);
    if(len > 0){
        eastl::string msg;
        msg.resize(len);
        FBXGetLastError(&msg[0]);
    }

    return "";
}


static inline void loadLib(){
    library = LoadLibrary(TEXT("FBXModule.dll"));

    __FBXM_BIND_FUNC(library, FBXGetLastError);

    __FBXM_BIND_FUNC(library, FBXOpenFile);
    __FBXM_BIND_FUNC(library, FBXCloseFile);

    __FBXM_BIND_FUNC(library, FBXReadScene);

    __FBXM_BIND_FUNC(library, FBXGetRootNode);
}
static inline void closeLib(){
    if(!FBXCloseFile()){
        printf_s("%s", getLastError().c_str());
        return;
    }

    FreeLibrary(library);
}


static inline void loadFile(const char* name){
    if(!FBXOpenFile(name, "rb")){
        printf_s("%s", getLastError().c_str());
        return;
    }
    
    if(!FBXReadScene()){
        printf_s("%s", getLastError().c_str());
        return;
    }
}

static inline size_t _getTriangleCount(const FBXNode* pNode){
    switch(pNode->getID()){
    case FBXType::FBXType_Mesh:
    case FBXType::FBXType_SkinnedMesh:
    {
        const auto* pMesh = static_cast<const FBXMesh*>(pNode);
        return pMesh->Indices.Length;
    }
    }

    return 0;
}
template<typename FUNC> static void _iterateNode(const FBXNode* pNode, DirectX::XMMATRIX matParent, FUNC func){
    func(matParent, pNode);

    if(pNode->Sibling)
        _iterateNode(pNode->Sibling, matParent, func);

    auto matCurrent = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pNode->TransformMatrix.Values);
    matCurrent = DirectX::XMMatrixMultiply(matCurrent, matParent);

    if(pNode->Child)
        _iterateNode(pNode->Child, matCurrent, func);
}
static inline void storeNode(const char* name){
    auto* pRootNode = reinterpret_cast<FBXNode*>(FBXGetRootNode());

    FILE* pFile;
    fopen_s(&pFile, name, "wb");
    if(!pFile)
        return;

    const unsigned char header[80] = { 0x80, };
    fwrite(header, sizeof(header), 1, pFile);

    auto countPos = ftell(pFile);
    unsigned long triangleCount = 0;
    fwrite(&triangleCount, sizeof(triangleCount), 1, pFile);

    _iterateNode(pRootNode, DirectX::XMMatrixIdentity(), [&](DirectX::XMMATRIX matParent, const FBXNode* pCurrent){
        auto matCurrent = DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)pCurrent->TransformMatrix.Values), matParent);

        switch(pCurrent->getID()){
        case FBXType::FBXType_Mesh:
        {
            const auto* pMesh = static_cast<const FBXMesh*>(pCurrent);

            const auto& commonVert = pMesh->Vertices;
            const auto& layeredVert = pMesh->LayeredVertices.Values[0];

            for(const auto* indices = pMesh->Indices.Values; size_t(indices - pMesh->Indices.Values) < pMesh->Indices.Length; ++indices){
                DirectX::XMVECTOR normal;
                {
                    auto normal0 = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&layeredVert.Normal.Values[indices->Values[0]]);
                    auto normal1 = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&layeredVert.Normal.Values[indices->Values[1]]);
                    auto normal2 = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&layeredVert.Normal.Values[indices->Values[2]]);

                    normal = DirectX::XMVectorAdd(normal0, normal1);
                    normal = DirectX::XMVectorAdd(normal, normal2);
                    normal = DirectX::XMVectorDivide(normal, DirectX::XMVectorReplicate(3.f));
                    normal = DirectX::XMVector3TransformNormal(normal, matCurrent);
                    normal = DirectX::XMVector3Normalize(normal);
                }

                DirectX::XMVECTOR pos0;
                DirectX::XMVECTOR pos1;
                DirectX::XMVECTOR pos2;
                {
                    pos0 = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&commonVert.Values[indices->Values[0]]);
                    pos1 = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&commonVert.Values[indices->Values[1]]);
                    pos2 = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&commonVert.Values[indices->Values[2]]);

                    pos0 = DirectX::XMVector3TransformCoord(pos0, matCurrent);
                    pos1 = DirectX::XMVector3TransformCoord(pos1, matCurrent);
                    pos2 = DirectX::XMVector3TransformCoord(pos2, matCurrent);
                }

                {
                    DirectX::XMFLOAT4A raw;

                    if((_mm_movemask_ps(DirectX::XMVectorIsNaN(normal)) & 0x07))
                        printf("NaN\n");
                    if((_mm_movemask_ps(DirectX::XMVectorIsNaN(pos0)) & 0x07))
                        printf("NaN\n");
                    if((_mm_movemask_ps(DirectX::XMVectorIsNaN(pos1)) & 0x07))
                        printf("NaN\n");
                    if((_mm_movemask_ps(DirectX::XMVectorIsNaN(pos2)) & 0x07))
                        printf("NaN\n");

                    DirectX::XMStoreFloat4A(&raw, normal);
                    fwrite(&raw, sizeof(float), 3, pFile);

                    DirectX::XMStoreFloat4A(&raw, pos0);
                    fwrite(&raw, sizeof(float), 3, pFile);

                    DirectX::XMStoreFloat4A(&raw, pos1);
                    fwrite(&raw, sizeof(float), 3, pFile);

                    DirectX::XMStoreFloat4A(&raw, pos2);
                    fwrite(&raw, sizeof(float), 3, pFile);
                }

                const unsigned short attribute = 0;
                fwrite(&attribute, sizeof(attribute), 1, pFile);

                ++triangleCount;
            }
            break;
        }
        case FBXType::FBXType_SkinnedMesh:
        {
            const auto* pMesh = static_cast<const FBXSkinnedMesh*>(pCurrent);

            const auto& commonVert = pMesh->Vertices;
            const auto& layeredVert = pMesh->LayeredVertices.Values[0];

            eastl::unordered_map<FBXNode*, DirectX::XMFLOAT4X4> skinDeforms;
            for(const auto* i = pMesh->SkinDeforms.Values; size_t(i - pMesh->SkinDeforms.Values) < pMesh->SkinDeforms.Length; ++i){
                auto matDeform = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)i->DeformMatrix.Values);

                DirectX::XMFLOAT4X4 raw;
                DirectX::XMStoreFloat4x4(&raw, matDeform);

                skinDeforms.emplace(i->TargetNode, raw);
            }

            for(const auto* indices = pMesh->Indices.Values; size_t(indices - pMesh->Indices.Values) < pMesh->Indices.Length; ++indices){
                DirectX::XMMATRIX matSkin[3];
                for(size_t i = 0; i < 3; ++i){
                    matSkin[i].r[0] = DirectX::XMVectorZero();
                    matSkin[i].r[1] = DirectX::XMVectorZero();
                    matSkin[i].r[2] = DirectX::XMVectorZero();
                    matSkin[i].r[3] = DirectX::XMVectorZero();

                    const auto& skinInfo = pMesh->SkinInfos.Values[indices->Values[i]];
                    for(const auto* p = skinInfo.Values; size_t(p - skinInfo.Values) < skinInfo.Length; ++p){
                        auto f = skinDeforms.find(p->BindNode);
                        if(f == skinDeforms.end())
                            continue;

                        auto m = DirectX::XMLoadFloat4x4(&f->second);
                        matSkin[i].r[0] = DirectX::XMVectorMultiplyAdd(m.r[0], DirectX::XMVectorReplicate(p->Weight), matSkin[i].r[0]);
                        matSkin[i].r[1] = DirectX::XMVectorMultiplyAdd(m.r[1], DirectX::XMVectorReplicate(p->Weight), matSkin[i].r[1]);
                        matSkin[i].r[2] = DirectX::XMVectorMultiplyAdd(m.r[2], DirectX::XMVectorReplicate(p->Weight), matSkin[i].r[2]);
                        matSkin[i].r[3] = DirectX::XMVectorMultiplyAdd(m.r[3], DirectX::XMVectorReplicate(p->Weight), matSkin[i].r[3]);
                    }

                    matSkin[i] = DirectX::XMMatrixMultiply(matSkin[i], matCurrent);
                }

                DirectX::XMVECTOR normal;
                {
                    auto normal0 = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&layeredVert.Normal.Values[indices->Values[0]]);
                    auto normal1 = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&layeredVert.Normal.Values[indices->Values[1]]);
                    auto normal2 = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&layeredVert.Normal.Values[indices->Values[2]]);

                    normal0 = DirectX::XMVector3TransformNormal(normal0, matSkin[0]);
                    normal1 = DirectX::XMVector3TransformNormal(normal1, matSkin[1]);
                    normal2 = DirectX::XMVector3TransformNormal(normal2, matSkin[2]);

                    normal = DirectX::XMVectorAdd(normal0, normal1);
                    normal = DirectX::XMVectorAdd(normal, normal2);
                    normal = DirectX::XMVectorDivide(normal, DirectX::XMVectorReplicate(3.f));
                    normal = DirectX::XMVector3Normalize(normal);
                }

                DirectX::XMVECTOR pos0;
                DirectX::XMVECTOR pos1;
                DirectX::XMVECTOR pos2;
                {
                    pos0 = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&commonVert.Values[indices->Values[0]]);
                    pos1 = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&commonVert.Values[indices->Values[1]]);
                    pos2 = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&commonVert.Values[indices->Values[2]]);

                    pos0 = DirectX::XMVector3TransformCoord(pos0, matSkin[0]);
                    pos1 = DirectX::XMVector3TransformCoord(pos1, matSkin[1]);
                    pos2 = DirectX::XMVector3TransformCoord(pos2, matSkin[2]);
                }

                {
                    DirectX::XMFLOAT4A raw;

                    if((_mm_movemask_ps(DirectX::XMVectorIsNaN(normal)) & 0x07))
                        printf("NaN\n");
                    if((_mm_movemask_ps(DirectX::XMVectorIsNaN(pos0)) & 0x07))
                        printf("NaN\n");
                    if((_mm_movemask_ps(DirectX::XMVectorIsNaN(pos1)) & 0x07))
                        printf("NaN\n");
                    if((_mm_movemask_ps(DirectX::XMVectorIsNaN(pos2)) & 0x07))
                        printf("NaN\n");

                    DirectX::XMStoreFloat4A(&raw, normal);
                    fwrite(&raw, sizeof(float), 3, pFile);

                    DirectX::XMStoreFloat4A(&raw, pos0);
                    fwrite(&raw, sizeof(float), 3, pFile);

                    DirectX::XMStoreFloat4A(&raw, pos1);
                    fwrite(&raw, sizeof(float), 3, pFile);

                    DirectX::XMStoreFloat4A(&raw, pos2);
                    fwrite(&raw, sizeof(float), 3, pFile);
                }

                const unsigned short attribute = 0;
                fwrite(&attribute, sizeof(attribute), 1, pFile);

                ++triangleCount;
            }

            break;
        }
        default:
            return;
        }
    });

    {
        fseek(pFile, countPos, SEEK_SET);
        fwrite(&triangleCount, sizeof(triangleCount), 1, pFile);
    }

    fclose(pFile);
}


int main(int argc, char* argv[]){
    loadLib();

    loadFile(argv[1]);
    storeNode(argv[2]);

    closeLib();
    return 0;
}