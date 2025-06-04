#include "meshGenerator.h"

// --------------------------------------------------------------------------------------------------------------------------

cMeshGenerator::sMeshData cMeshGenerator::CreateCylinder(float _bottomRadius, float _topRadius, float _height, uint32 _sliceCount, uint32 _stackCount)
{
	sMeshData meshData; 

	float stackHeight = _height / _stackCount; 

	float radiusStep = (_topRadius - _bottomRadius) / _stackCount;

	uint32 ringCount = _stackCount + 1; 

	// compute vertecies
	for (uint32 stackIndex = 0; stackIndex < ringCount; stackIndex++)
	{
		float y = -0.5f * _height + stackIndex * stackHeight;
		float r = _bottomRadius + stackIndex * radiusStep;

		float dTheta = 2.f * XM_PI / _sliceCount;
		for (uint32 sliceIndex = 0; sliceIndex <= _sliceCount; sliceIndex++)
		{
			sVertex vertex;

			float c = cosf(sliceIndex * dTheta);
			float s = sinf(sliceIndex * dTheta);

			vertex.position = XMFLOAT3(r * c, y, r * s);

			vertex.texC.x = static_cast<float>(sliceIndex / _sliceCount);
			vertex.texC.y = 1.f - static_cast<float>(stackIndex / _stackCount);

			vertex.tangentU = XMFLOAT3(-s, 0.f, c);

			float dr = _bottomRadius - _topRadius;
			XMFLOAT3 bitangent(dr * c, -_height, dr * s);

			XMVECTOR T = XMLoadFloat3(&vertex.tangentU);
			XMVECTOR B = XMLoadFloat3(&bitangent);
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			XMStoreFloat3(&vertex.normal, N);

			meshData.vertecies.push_back(vertex);
		}
	}

	uint32 ringVertexCount = _sliceCount + 1;
	
	for (uint32 stackIndex = 0; stackIndex < _stackCount; ++stackIndex)
	{
		for (uint32 sliceIndex = 0; sliceIndex < _sliceCount; ++sliceIndex)
		{
			uint32 current = stackIndex * ringVertexCount + sliceIndex;
			uint32 next = (stackIndex + 1) * ringVertexCount + sliceIndex;

			// Triangle 1
			meshData.indices32.push_back(current);
			meshData.indices32.push_back(next);
			meshData.indices32.push_back(next + 1);

			// Triangle 2
			meshData.indices32.push_back(current);
			meshData.indices32.push_back(next + 1);
			meshData.indices32.push_back(current + 1);
		}
	}

	BuildCylinderTopCap(_topRadius, _height, _sliceCount, meshData);
	BuildCylinderBottomCap(_bottomRadius, _height, _sliceCount, meshData);

	return meshData;
}

// --------------------------------------------------------------------------------------------------------------------------

void cMeshGenerator::BuildCylinderTopCap(float _topRadius, float _height, uint32 _sliceCount, sMeshData& meshData)
{
	uint32 baseIndex = static_cast<uint32>(meshData.vertecies.size());
	float y = 0.5f * _height;
	float dTheta = 2.f * XM_PI / _sliceCount;

	for (uint32 sliceIndex = 0; sliceIndex <= _sliceCount; ++sliceIndex)
	{
		float x = _topRadius * cosf(sliceIndex * dTheta);
		float z = _topRadius * sinf(sliceIndex * dTheta);

		float u = x / (2.0f * _topRadius) + 0.5f;
		float v = z / (2.0f * _topRadius) + 0.5f;

		sVertex vertex;
		vertex.position = XMFLOAT3(x, y, z);
		vertex.normal = XMFLOAT3(0.f, 1.f, 0.f);
		vertex.tangentU = XMFLOAT3(1.f, 0.f, 0.f); // arbitrary direction in tangent plane
		vertex.texC = XMFLOAT2(u, v);

		meshData.vertecies.push_back(vertex);
	}

	// Center vertex
	sVertex centerVertex;
	centerVertex.position = XMFLOAT3(0.f, y, 0.f);
	centerVertex.normal = XMFLOAT3(0.f, 1.f, 0.f);
	centerVertex.tangentU = XMFLOAT3(1.f, 0.f, 0.f);
	centerVertex.texC = XMFLOAT2(0.5f, 0.5f);

	meshData.vertecies.push_back(centerVertex);

	uint32 centerIndex = static_cast<uint32>(meshData.vertecies.size() - 1);

	for (uint32 sliceIndex = 0; sliceIndex < _sliceCount; ++sliceIndex)
	{
		meshData.indices32.push_back(centerIndex);
		meshData.indices32.push_back(baseIndex + sliceIndex + 1);
		meshData.indices32.push_back(baseIndex + sliceIndex);
	}
}

// --------------------------------------------------------------------------------------------------------------------------

void cMeshGenerator::BuildCylinderBottomCap(float _bottomRadius, float _height, uint32 _sliceCount, sMeshData& meshData)
{
	uint32 baseIndex = static_cast<uint32>(meshData.vertecies.size());
	float y = -0.5f * _height;
	float dTheta = 2.f * XM_PI / _sliceCount;

	for (uint32 sliceIndex = 0; sliceIndex <= _sliceCount; ++sliceIndex)
	{
		float x = _bottomRadius * cosf(sliceIndex * dTheta);
		float z = _bottomRadius * sinf(sliceIndex * dTheta);

		float u = x / _height + 0.5f;
		float v = z / _height + 0.5f;

		sVertex vertex;
		vertex.position = XMFLOAT3(x, y, z);
		vertex.normal = XMFLOAT3(0.f, -1.f, 0.f);
		vertex.tangentU = XMFLOAT3(1.f, 0.f, 0.f);
		vertex.texC = XMFLOAT2(u, v);

		meshData.vertecies.push_back(vertex);
	}

	sVertex centerVertex;
	centerVertex.position = XMFLOAT3(0.f, y, 0.f);
	centerVertex.normal = XMFLOAT3(0.f, -1.f, 0.f);
	centerVertex.tangentU = XMFLOAT3(1.f, 0.f, 0.f);
	centerVertex.texC = XMFLOAT2(0.5f, 0.5f);

	meshData.vertecies.push_back(centerVertex);

	uint32 centerIndex = static_cast<uint32>(meshData.vertecies.size() - 1);

	for (uint32 sliceIndex = 0; sliceIndex < _sliceCount; ++sliceIndex)
	{
		meshData.indices32.push_back(centerIndex);
		meshData.indices32.push_back(baseIndex + sliceIndex);
		meshData.indices32.push_back(baseIndex + sliceIndex + 1);
	}
}
