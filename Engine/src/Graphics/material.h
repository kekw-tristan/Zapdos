#pragma once

#include <DirectXMath.h>

using namespace DirectX;

struct sMaterial
{
    XMFLOAT3 albedo;     // Base color
    float alpha;         // Opacity

    float metallic;      // 0 = dielectric, 1 = metal
    float roughness;     // 0 = smooth, 1 = rough
    float ao;            // Ambient occlusion factor

    XMFLOAT3 emissive;   // Emissive color
    
    int baseColorIndex;       
    int metallicRoughnessIndex;       
    int normalIndex;       
    int occlusionIndex;       
    int emissiveIndex;          

    float normalScale;
    float occlusionStrength;

    // Constructor with default values
    sMaterial()
        : albedo(1.0f, 1.0f, 1.0f)
        , alpha(1.0f)
        , metallic(0.0f)
        , roughness(0.5f)
        , ao(1.0f)
        , emissive(0.0f, 0.0f, 0.0f)
        , baseColorIndex(0.0f)
        , metallicRoughnessIndex(0.0f)
        , normalIndex(0.0f)
        , occlusionIndex(0.0f)
        , emissiveIndex(0.0f)
        , normalScale(0.0f)
        , occlusionStrength(0.0f)
    {}
};

