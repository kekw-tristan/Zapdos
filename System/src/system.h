#pragma once 

#include <windows.h>

class cWindow;

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

		cWindow* pWindow; 
		cWindow* pWindow2; 
};