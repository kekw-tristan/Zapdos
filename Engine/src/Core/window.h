#pragma once

#include <Windows.h>

class cTimer;

class cWindow
{
	public:

		cWindow()	= default; 
		~cWindow()	= default;

	public:

		void Initialize(const wchar_t* _pTitle, const wchar_t* _className, int _width, int _height, cTimer* _pTimer);
		void MessageHandling();

	public:

		bool GetIsRunning(); 

		int GetWidth();
		int GetHeight();

		HWND GetHWND();

		bool GetIsResizing();
		
		bool GetHasResized();
		void SetHasResized(bool _hasResized);

		bool GetIsWindowPaused();

		bool GetIsFullscreen();
		

	private:

		static LRESULT CALLBACK WindowProcStatic(HWND _hwnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam);
		LRESULT CALLBACK WindowProc(UINT _uMsg, WPARAM _wParam, LPARAM _lParam);

	public:

		void EnterFullscreen();
		void ExitFullscreen();

	private:

		HWND		m_hwnd; 
		HINSTANCE	m_hInstance;

		bool m_isRunning; 

		int m_width;
		int m_height;

		cTimer* m_pTimer;

		bool m_isResizing;
		bool m_hasResized; 
		bool m_isWindowPaused;
		
		bool m_isFullscreen;
		WINDOWPLACEMENT m_prevWindowPlacement; 
		DWORD m_windowStyle;

};
