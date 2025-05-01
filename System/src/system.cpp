#include "system.h"

#include <iostream>

#include "window.h"
#include "directx12.h"
#include "timer.h"
#include "input.h"


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
		// check if paused		
		if (m_pWindow->GetIsWindowPaused())
			continue;


		if (m_pWindow->GetHasResized())
		{
			m_pDirectX12->OnResize();
			m_pWindow->SetHasResized(false);
		}
		

		m_pWindow->MessageHandling();
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
	delete m_pDirectX12;
	delete m_pTimer;
}

// --------------------------------------------------------------------------------------------------------------------------
