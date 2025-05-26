#include "window.h"

#include <iostream>
#include <windowsx.h>

#include "input.h"
#include "timer.h"

// --------------------------------------------------------------------------------------------------------------------------

void cWindow::Initialize(const wchar_t* _pTitle, const wchar_t* _pClassName, int _width, int _height, cTimer* _pTimer)
{
	m_height = _height;
	m_width  = _width;

	m_pTimer = _pTimer;

	m_hInstance = GetModuleHandle(nullptr);

	WNDCLASSEX wndc = {};

	wndc.cbSize = sizeof(wndc);
	wndc.hInstance = m_hInstance;
	wndc.lpszClassName = _pClassName;
	wndc.lpfnWndProc = cWindow::WindowProcStatic;

	RegisterClassEx(&wndc);

	RECT windowRect = { 0, 0, m_width, m_height };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int actualWidth = windowRect.right - windowRect.left;
	int actualHeight = windowRect.bottom - windowRect.top;

	m_hwnd = CreateWindowEx(
		0,
		_pClassName,
		_pTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, actualWidth, actualHeight,
		NULL, NULL, m_hInstance, this
	);

	ShowWindow(m_hwnd, SW_SHOW);
	UpdateWindow(m_hwnd);

	m_isRunning = true;
	m_isFullscreen = false; 

	m_windowStyle = GetWindowLong(m_hwnd, GWL_STYLE);
}

// --------------------------------------------------------------------------------------------------------------------------

void cWindow::MessageHandling()
{
	MSG msg = {};

	if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

// --------------------------------------------------------------------------------------------------------------------------

bool cWindow::GetIsRunning()
{
	return m_isRunning;
}

// --------------------------------------------------------------------------------------------------------------------------

int cWindow::GetWidth()
{
	return m_width;
}

// --------------------------------------------------------------------------------------------------------------------------

int cWindow::GetHeight()
{
	return m_height;
}

// --------------------------------------------------------------------------------------------------------------------------

HWND cWindow::GetHWND()
{
	return m_hwnd;
}

// --------------------------------------------------------------------------------------------------------------------------

bool cWindow::GetIsResizing()
{
	return m_isResizing;
}

// --------------------------------------------------------------------------------------------------------------------------

bool cWindow::GetHasResized()
{
	return m_hasResized;
}

// --------------------------------------------------------------------------------------------------------------------------

void cWindow::SetHasResized(bool _hasResized)
{
	m_hasResized = _hasResized;
}

// --------------------------------------------------------------------------------------------------------------------------

bool cWindow::GetIsWindowPaused()
{
	return m_isWindowPaused;
}

bool cWindow::GetIsFullscreen()
{
	return m_isFullscreen;
}

// --------------------------------------------------------------------------------------------------------------------------

LRESULT cWindow::WindowProcStatic(HWND _hwnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
{
	cWindow* pWindow = nullptr; 

	if (_uMsg == WM_NCCREATE)
	{
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(_lParam); 
		pWindow = reinterpret_cast<cWindow*>(pCreate->lpCreateParams);
		SetWindowLongPtr(_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));

		pWindow->m_hwnd = _hwnd;
	}
	else
	{
		pWindow = reinterpret_cast<cWindow*>(GetWindowLongPtr(_hwnd, GWLP_USERDATA));
	}

	if (pWindow)
	{
		return pWindow->WindowProc(_uMsg, _wParam, _lParam);
	}

	return DefWindowProc(_hwnd, _uMsg, _wParam, _lParam);
}

// --------------------------------------------------------------------------------------------------------------------------

LRESULT cWindow::WindowProc(UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
{
	switch (_uMsg)
	{
		case WM_DESTROY:
			m_isRunning = false;
			break; 

		case WM_CLOSE:
			DestroyWindow(m_hwnd);
			break;

		case WM_ACTIVATE:
			if (LOWORD(_wParam) == WA_INACTIVE)
			{
				m_isWindowPaused = true;
				m_pTimer->Stop();
			}
			else
			{
				m_isWindowPaused = false; 
				m_pTimer->Start();
			}
			break;

		case WM_ENTERSIZEMOVE:
			m_isWindowPaused	= true;
			m_isResizing		= true;
			m_pTimer->Stop();
			break;

		case WM_EXITSIZEMOVE:
			m_isWindowPaused	= false;
			m_isResizing		= false;
			m_pTimer->Start();
			break;

		case WM_SIZE:
			m_hasResized = true;

			m_width		= LOWORD(_lParam);  
			m_height	= HIWORD(_lParam);
			break;

		case WM_KEYDOWN:
			if (_wParam == VK_F11)
			{
				if (m_isFullscreen)
					ExitFullscreen();
				else
					EnterFullscreen();
			}
			cInput::OnKeyDown(_wParam);
			break;

		case WM_KEYUP:
			cInput::OnKeyUp(_wParam);
			break;

		case WM_MOUSEMOVE:
			cInput::OnMouseMove(GET_X_LPARAM(_lParam), GET_Y_LPARAM(_lParam));
			break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			cInput::OnMouseButtonDown(_wParam);
			break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
			cInput::OnMouseButtonUp(_wParam);
			break;

		case WM_SYSCHAR:
			// Suppress beep for system characters (like Alt+Enter)
			// Fullscreen toggled with f11 (only borderless fullscreen)
			if (_wParam == VK_RETURN)
			{
				// skips default proc so no windows sound
				return 0; 			
			}
			break;
	}

	return DefWindowProc(m_hwnd, _uMsg, _wParam, _lParam);
}

// --------------------------------------------------------------------------------------------------------------------------

void cWindow::EnterFullscreen()
{
	m_isFullscreen	= true;
	m_hasResized	= true;

	// Save current window placement before changing
	MONITORINFO mi = { sizeof(mi) };
	if (GetWindowPlacement(m_hwnd, &m_prevWindowPlacement) &&
		GetMonitorInfo(MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTOPRIMARY), &mi))
	{
		// Change to borderless fullscreen
		SetWindowLong(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(
			m_hwnd, HWND_TOP,
			mi.rcMonitor.left, mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED
		);
	}

	RECT rect;
	GetClientRect(m_hwnd, &rect);
	m_width = rect.right - rect.left;
	m_height = rect.bottom - rect.top;
}

// --------------------------------------------------------------------------------------------------------------------------

void cWindow::ExitFullscreen()
{
	m_isFullscreen	= false;
	m_hasResized	= true;


	// Restore window style and placement
	SetWindowLong(m_hwnd, GWL_STYLE, m_windowStyle);
	SetWindowPlacement(m_hwnd, &m_prevWindowPlacement);
	SetWindowPos(
		m_hwnd, nullptr, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
		SWP_NOOWNERZORDER | SWP_FRAMECHANGED
	);

	RECT rect;
	GetClientRect(m_hwnd, &rect);
	m_width = rect.right - rect.left;
	m_height = rect.bottom - rect.top;
}

// --------------------------------------------------------------------------------------------------------------------------
