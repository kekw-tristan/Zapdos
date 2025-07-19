#pragma once 

#include <d3d12.h>
#include <DirectXMath.h>

#include "directx12.h"   
#include "material.h"

using namespace DirectX;

struct sRenderItem
{
    sRenderItem()
        : worldMatrix()
        , numberOfFramesDirty(c_NumberOfFrameResources)
        , objCBIndex(-1)
        , pGeometry(nullptr)
        , pMaterial(nullptr)
        , primitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
        , indexCount(0)
        , startIndexLocation(0)
        , baseVertexLocation(0)
    {
        XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
    }

    XMFLOAT4X4                  worldMatrix;
    int                         numberOfFramesDirty;  
    UINT                        objCBIndex;          
    sMeshGeometry*              pGeometry;          
    sMaterial*                  pMaterial;         

    D3D12_PRIMITIVE_TOPOLOGY    primitiveType;    
    UINT                        indexCount;      
    UINT                        startIndexLocation;   
    int                         baseVertexLocation;  
};
