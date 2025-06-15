#pragma once

#include <DirectXMath.h>

using namespace DirectX; 

struct sDirectionalLightConstants
{
    XMFLOAT3 direction;    // 12 bytes
    float intensity;       // 4 bytes
    XMFLOAT3 color;        // 12 bytes  
    float padding;         // 4 bytes
};
