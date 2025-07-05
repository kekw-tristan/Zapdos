#pragma once 

#include <cstdint>
#include <windows.h>
#include <DirectXMath.h>
#include <vector>

#include <graphics/renderItem.h>
#include <graphics/light.h>

using namespace DirectX;

struct sVertex; 

class cWindow;
class cDirectX12;
class cTimer; 

constexpr int c_numberOfRenderItems = 10000;

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

		void InitializeRenderItems(); 
		void InitializeLights();
		void InitializeLightVisualizations();

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

		XMFLOAT4X4 m_view;
		XMFLOAT4X4 m_world;
		XMFLOAT4X4 m_proj;

		std::vector<sRenderItem> m_renderItems;
		std::vector<sLightConstants> m_lights;
		std::unordered_map<std::string, sMaterial> m_materials;

		XMFLOAT3	m_position	= XMFLOAT3(0.0f, 0.0f, -30.0f);
		float		m_yaw		= 0.0f;    
		float		m_pitch		= 0.0f;

		size_t m_lightVisStartIndex = 0; 
};