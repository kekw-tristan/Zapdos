#pragma once 

#include <windows.h>
#include <DirectXMath.h>

using namespace DirectX;

class cWindow;
class cDirectX12;
class cTimer; 

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

		float m_radius;

		int m_lastMouseX;
		int m_lastMouseY;

		XMFLOAT4X4 m_view;
		XMFLOAT4X4 m_world;
		XMFLOAT4X4 m_proj;
};