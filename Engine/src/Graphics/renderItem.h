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
    int                         numberOfFramesDirty;  // Tracks how many frames need constant buffer updates
    UINT                        objCBIndex;           // Index into the constant buffer for this object
    sMeshGeometry*              pGeometry;            // Pointer to geometry data
    sMaterial*                  pMaterial;            // Pointer to material used by this render item

    D3D12_PRIMITIVE_TOPOLOGY    primitiveType;        // Usually triangle list
    UINT                        indexCount;           // Number of indices to draw
    UINT                        startIndexLocation;   // Start index location in the index buffer
    int                         baseVertexLocation;   // Base vertex location for indexed drawing
};
