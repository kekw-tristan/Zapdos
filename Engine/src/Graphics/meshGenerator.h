#pragma once

#include <cstdint>
#include <DirectXMath.h>
#include <vector>

using namespace DirectX; 

using uint16 = std::uint16_t;
using uint32 = std::uint32_t;

class cMeshGenerator
{
	public:

		struct sVertex
		{
			sVertex()
				: position()
				, normal()
				, tangentU()
				, texC() {};
			sVertex(const XMFLOAT3& _pos, const XMFLOAT3& _normal, const DirectX::XMFLOAT3& _tangentU, const XMFLOAT2& _uv)
				: position(_pos)
				, normal(_normal)
				, tangentU(_tangentU)
				, texC(_uv)
			{}
			XMFLOAT3 position;
			XMFLOAT3 normal;
			XMFLOAT3 tangentU;
			XMFLOAT2 texC;
		};

		struct sMeshData
		{
			public:
				std::vector<sVertex> vertecies;
				std::vector<uint32> indices32;

				std::vector<uint16>& GetIndices16()
				{
					if (m_indices16.empty())
					{
						m_indices16.resize(indices32.size());
						for (size_t i = 0; i < indices32.size(); ++i)
							m_indices16[i] = static_cast<uint16>(indices32[i]);
					}
					return m_indices16;
				}

			private:

				std::vector<uint16> m_indices16;

		};

	public:

		sMeshData CreateCylinder(float _bottomRadius, float _topRadius, float _height, uint32 _sliceCount, uint32 _stackCount); 
		sMeshData CreateCube();
		sMeshData CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount);
		

	private:

		void BuildCylinderTopCap(float _topRadius, float _height, uint32 _sliceCount, sMeshData& meshData);
		void BuildCylinderBottomCap(float _topRadius, float _height, uint32 _sliceCount, sMeshData& meshData);
};
