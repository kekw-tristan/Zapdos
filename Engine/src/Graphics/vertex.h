#pragma once

#include <DirectXMath.h>

using namespace DirectX;

struct sVertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT4 tangentU;
    XMFLOAT2 texC;
    XMFLOAT2 texC2; 
};