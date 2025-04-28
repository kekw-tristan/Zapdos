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

HWND cWindow::GetHWND()
{
	return m_hwnd;
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

		std::cout << "hwnd set\n";
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
				m_pTimer->Stop();
			}
			else
			{
				m_pTimer->Start();
			}
			break;

		case WM_KEYDOWN:
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
	}

	return DefWindowProc(m_hwnd, _uMsg, _wParam, _lParam);
}

// --------------------------------------------------------------------------------------------------------------------------
