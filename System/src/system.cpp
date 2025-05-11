#include "system.h"

#include <algorithm>
#include <iostream>
#include <DirectXColors.h>
#include <wrl.h>
#include <d3d12.h>

#include "window.h"
#include "directx12.h"
#include "directx12Util.h"
#include "timer.h"
#include "input.h"
#include "vertex.h"

using namespace DirectX;
using namespace Microsoft::WRL;


// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Initialize()
{
	std::cout << "Initialize" << "\n";
	m_pTimer = new cTimer();
	m_pTimer->Start();
	
	m_pWindow = new cWindow();
	m_pWindow->Initialize(L"Zapdos", L"gameWindow", 1280, 720, m_pTimer);

	m_pDirectX12 = new cDirectX12();
	m_pDirectX12->Initialize(m_pWindow, m_pTimer);



}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Run()
{
	while (m_pWindow->GetIsRunning())
	{
		m_pWindow->MessageHandling();

		// check if paused		
		if (m_pWindow->GetIsWindowPaused())
			continue;


		if (m_pWindow->GetHasResized())
		{
			m_pDirectX12->OnResize();
			m_pWindow->SetHasResized(false);
		}
		
		m_pTimer->Tick();

		m_pDirectX12->CalculateFrameStats();
		m_pDirectX12->Draw();
	}	

}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Finalize()
{
	std::cout << "Finalize" << "\n";

	delete m_pWindow;

	m_pDirectX12->Finalize();
	delete m_pDirectX12;
	delete m_pTimer;
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Update()
{
	HandleInput();

	float x = m_radius * sinf(m_phi) * cosf(m_theta);
	float y = m_radius * sinf(m_phi) * sinf(m_theta);
	float z = m_radius * cosf(m_phi); 

	XMVECTOR pos = XMVectorSet(x, y, z, 1.f);
	XMVECTOR at = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	XMMATRIX view = XMMatrixLookAtLH(pos, at, up);

	XMStoreFloat4x4(&m_view, view);


	
	m_pDirectX12->Update(view);
	
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::HandleInput()
{
	if (cInput::IsMouseButtonDown(MK_LBUTTON))
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(cInput::GetMouseX() - m_lastMouseX));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(cInput::GetMouseY() - m_lastMouseY));

		m_theta += dx;
		m_phi += dy;

		m_phi = std::clamp(m_phi, 0.1f, c_pi - 0.1f);
	}

	else if (cInput::IsMouseButtonDown(MK_RBUTTON))
	{
		float dx = 0.005f * XMConvertToRadians(0.25f * static_cast<float>(cInput::GetMouseX() - m_lastMouseX));
		float dy = 0.005f * XMConvertToRadians(0.25f * static_cast<float>(cInput::GetMouseY() - m_lastMouseY));

		m_radius = std::clamp(m_radius, 3.f, 15.f);
	}

	m_lastMouseX = cInput::GetMouseX();
	m_lastMouseY = cInput::GetMouseY();
}
