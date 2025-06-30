#pragma once

#include <DirectXMath.h>
#include <string>
#include "directx12.h"

using namespace DirectX;

struct sMaterialConstants
{
    XMFLOAT4 diffuseAlbedo;
    XMFLOAT3 frenselR0;
    XMFLOAT4X4 matTransform;
    float roughness;
    sMaterialConstants()
        : diffuseAlbedo({ 1.f, 1.f, 1.f, 1.f })
        , frenselR0({ 0.01f, 0.01f, 0.01f })
        , roughness(0.25)
        , matTransform()
    {
        XMStoreFloat4x4(&matTransform, XMMatrixIdentity());
    }
        
};

struct sMaterial
{
    std::string name;
    int matCBIndex; 

    // texturing
    int diffuseSrvHeapIndex; 
    int numberOfFramesDirty; 

    sMaterialConstants materialConstants; 

    sMaterial()
        : name()
        , matCBIndex(-1)
        , diffuseSrvHeapIndex(-1)
        , numberOfFramesDirty(c_NumberOfFrameResources)
        , materialConstants()
    {
    }
};
