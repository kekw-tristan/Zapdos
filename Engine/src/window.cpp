#include "window.h"

#include <iostream>

// --------------------------------------------------------------------------------------------------------------------------

void cWindow::Initialize(const wchar_t* _pTitle, const wchar_t* _pClassName, int _width, int _height)
{
	m_height	= _height;
	m_width = _width;

	hInstance = GetModuleHandle(nullptr);

	WNDCLASSEX wndc = {};

	wndc.cbSize = sizeof(wndc);
	wndc.hInstance = hInstance; 
	wndc.lpszClassName = _pClassName;
	wndc.lpfnWndProc = cWindow::WindowProcStatic;

	RegisterClassEx(&wndc);

	hwnd = CreateWindowEx(
		0,
		_pClassName,
		_pTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height,
		NULL, NULL, hInstance, this
	);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	isRunning = true; 
}

// --------------------------------------------------------------------------------------------------------------------------

void cWindow::MessageHandling()
{
	MSG msg = {};

	GetMessage(&msg, nullptr, 0, 0);

	TranslateMessage(&msg);
	DispatchMessage(&msg);
}

// --------------------------------------------------------------------------------------------------------------------------

bool cWindow::GetIsRunning()
{
	return isRunning;
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
	return hwnd;
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
		pWindow->hwnd = _hwnd; 
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
		{
			isRunning = false;
			break; 
		}

		case WM_CLOSE:
		{
			DestroyWindow(hwnd); 
			break;
		}
	}

	return DefWindowProc(hwnd, _uMsg, _wParam, _lParam);
}

// --------------------------------------------------------------------------------------------------------------------------
