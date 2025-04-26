#pragma once

#include <Windows.h>

class cWindow
{
	public:

		cWindow()	= default; 
		~cWindow()	= default;

	public:

		void Initialize(const wchar_t* _pTitle, const wchar_t* _className, int _width, int _height); 
		void MessageHandling();

	public:

		bool GetIsRunning(); 

		int GetWidth();
		int GetHeight();

		HWND GetHWND();
	private:

		static LRESULT CALLBACK WindowProcStatic(HWND _hwnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam);
		LRESULT CALLBACK WindowProc(UINT _uMsg, WPARAM _wParam, LPARAM _lParam);

	private:

		HWND		hwnd; 
		HINSTANCE	hInstance;

		bool isRunning; 

		int m_width;
		int m_height;
};
