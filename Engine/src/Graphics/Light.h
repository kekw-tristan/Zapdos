#pragma once

#include <DirectXMath.h>

using namespace DirectX; 

class cLight
{
	XMFLOAT3 Strength;
	float FalloffStart;
	XMFLOAT3 Direction;
	float FalloffEnd;	
	XMFLOAT3 Position;
	float SpotPower;
};
