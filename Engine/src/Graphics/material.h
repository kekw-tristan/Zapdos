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
    float padding;       // pad to 16-byte alignment

    // Constructor with default values
    sMaterial()
        : albedo(1.0f, 1.0f, 1.0f)
        , alpha(1.0f)
        , metallic(0.0f)
        , roughness(0.5f)
        , ao(1.0f)
        , emissive(0.0f, 0.0f, 0.0f)
        , padding(0.0f)
    {}
};

