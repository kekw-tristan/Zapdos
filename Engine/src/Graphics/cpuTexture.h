#pragma once

#include <d3d12.h>
#include <vector>

class cCpuTexture
{
	public:

		cCpuTexture(int _width, int _height, std::vector<uint8_t> _data, DXGI_FORMAT _format = DXGI_FORMAT_R8G8B8A8_UNORM);
		~cCpuTexture();

	public:

		std::vector<uint8_t>& GetData();

		int GetWidth();
		int GetHeight(); 

		DXGI_FORMAT GetFormat();

	private:	

		int m_width;
		int m_height;

		std::vector<uint8_t>	m_data;
		DXGI_FORMAT				m_format;

};