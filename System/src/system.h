#pragma once 

#include <windows.h>

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

		void InitializeVertices();

	private:

		cWindow*	m_pWindow; 
		cDirectX12* m_pDirectX12; 
		cTimer*		m_pTimer;
};