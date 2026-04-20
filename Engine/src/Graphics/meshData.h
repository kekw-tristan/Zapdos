#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

#include "Graphics/vertex.h"

using uint16 = std::uint16_t;
using uint32 = std::uint32_t;

struct sMeshData
{
	public:
		std::vector<sVertex> vertices;
		std::vector<uint32> indices32;
		std::vector<uint16> indices16;
		int materialId = -1;
	
		std::vector<uint16>& GetIndices16()
		{
			if (indices16.empty())
			{
				indices16.resize(indices32.size());
	
				for (size_t i = 0; i < indices32.size(); ++i)
				{
					if (indices32[i] > UINT16_MAX)
					{
						throw std::runtime_error("Mesh uses indices > 65535; uint16 index buffer not possible.");
					}
	
					indices16[i] = static_cast<uint16>(indices32[i]);
				}
			}
	
			return indices16;
		}
};