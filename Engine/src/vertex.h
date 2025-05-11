#pragma once

#include <DirectXMath.h>

using namespace DirectX;

struct sVertex
{
	XMFLOAT3 pos;
	XMFLOAT4 color;

	sVertex(const XMFLOAT3& _pos, const XMFLOAT4& _color)
		: pos(pos), color(_color) {}
};
