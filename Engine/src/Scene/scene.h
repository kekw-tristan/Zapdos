#pragma once
#include <vector>

#include "Graphics/renderItem.h"
#include "Graphics/light.h"

class cScene
{
	public:
		std::vector<sRenderItem>&		GetRenderItems(); 
		std::vector<sLightConstants>&	GetLight();

	private:

		std::vector<sRenderItem>		m_renderItems;
		std::vector<sLightConstants>	m_lightConstants; 
};