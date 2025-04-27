#include "system.h"

#include <iostream>

#include "window.h"
#include "directx12.h"

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Initialize()
{
	std::cout << "Initialize" << "\n";

	pWindow = new cWindow();
	pWindow->Initialize(L"Zapdos", L"gameWindow", 1280, 720);

	pDirectX12 = new cDirectX12();
	pDirectX12->Initialize(pWindow); 
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Run()
{
	while (pWindow->GetIsRunning())
	{
		pWindow->MessageHandling();
	}
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Finalize()
{
	std::cout << "Finalize" << "\n";

	delete pWindow;
	delete pDirectX12; 
}

// --------------------------------------------------------------------------------------------------------------------------
