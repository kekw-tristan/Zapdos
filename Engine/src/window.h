#pragma once

#include <Windows.h>

class cWindow
{
	public:

		cWindow()	= default; 
		~cWindow()	= default;

	public:

		void Initialize(); 
		void MessageHandling();

	private:

		static LRESULT CALLBACK WindowProcStatic(HWND _hwnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam);
		LRESULT CALLBACK WindowProc(UINT _uMsg, WPARAM _wParam, LPARAM _lParam);

	private:

		HWND		hwnd; 
		HINSTANCE	hInstance;

		bool isRunning; 

};
