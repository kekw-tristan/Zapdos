#include "system.h"

#include <iostream>

#include "window.h"

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Initialize()
{
	std::cout << "Initialize" << "\n";

	pWindow = new cWindow();
	pWindow2 = new cWindow();
			
	pWindow->Initialize();
	pWindow2->Initialize();

	FreeConsole(); 
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Run()
{
	pWindow->MessageHandling();
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Finalize()
{
	std::cout << "Finalize" << "\n";
	delete pWindow;
	delete pWindow2;
}
