#pragma once

#include <DirectXMath.h>

using namespace DirectX; 

struct sLightConstants
{
	XMFLOAT3	strength;
	float		falloffStart;
	XMFLOAT3	direction;
	float		falloffEnd;	
	XMFLOAT3	position;
	float		spotPower;
};
