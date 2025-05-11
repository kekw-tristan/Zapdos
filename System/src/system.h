#pragma once 

#include <windows.h>
#include <DirectXMath.h>

using namespace DirectX;

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

		float m_theta = 1.5f * c_pi;
		float m_phi = XM_PIDIV4;
		float mRadius = 5.f;

		float m_radius;

		int m_lastMouseX;
		int m_lastMouseY;

		XMFLOAT4X4 m_view;
		XMFLOAT4X4 m_world;
		XMFLOAT4X4 m_proj;
};