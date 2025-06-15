#pragma once

#include <DirectXMath.h>

using namespace DirectX;

struct sVertex
{
    XMFLOAT3 pos;
    XMFLOAT3 normal;

    sVertex()
        : pos(), normal() {}

    sVertex(const XMFLOAT3& _pos, const XMFLOAT3& _normal)
        : pos(_pos), normal(_normal) {}
};