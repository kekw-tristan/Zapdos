#pragma once 

#include <cstdint>
#include <windows.h>
#include <DirectXMath.h>
#include <vector>

#include "Graphics/vertex.h"

using namespace DirectX;

struct sVertex; 

class cWindow;
class cDirectX12;
class cTimer; 

constexpr float c_pi = 3.1415927f;

class cSystem
{
	public:
		
		cSystem()	= default; 
		~cSystem()	= default;

	public:

		void Initialize();
		void Run(); 
		void Finalize(); 

	private:

		void Update();

		void HandleInput();

	private:

		cWindow*	m_pWindow; 
		cDirectX12* m_pDirectX12; 
		cTimer*		m_pTimer;

	private:

		float m_theta;
		float m_phi;
		float mRadius;

		float m_radius;

		int m_lastMouseX;
		int m_lastMouseY;

		std::vector<sVertex>		m_vertices;
		std::vector<std::int16_t>	m_indices;

		XMFLOAT4X4 m_view;
		XMFLOAT4X4 m_world;
		XMFLOAT4X4 m_proj;
};