#pragma once

#include <DirectXMath.h>

using namespace DirectX;

struct sMaterial
{
    DirectX::XMFLOAT3 albedo;         
    float specularExponent;          

    sMaterial()
        : albedo()  
        , specularExponent()  
    {}
};
