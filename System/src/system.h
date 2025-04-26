#pragma once 

#include <windows.h>

class cWindow;
class cDirectX12;

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

		cWindow*	pWindow; 
		cDirectX12* pDirectX12; 
};