#include "window.h"

// --------------------------------------------------------------------------------------------------------------------------

void cWindow::Initialize()
{
	hInstance = GetModuleHandle(nullptr);

	WNDCLASSEX wndc = {};

	wndc.cbSize = sizeof(wndc);
	wndc.hInstance = hInstance; 
	wndc.lpszClassName = L"gameWindow";
	wndc.lpfnWndProc = cWindow::WindowProcStatic;

	RegisterClassEx(&wndc);

	hwnd = CreateWindowEx(
		0,
		L"gameWindow",
		L"Zapdos",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
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

LRESULT cWindow::WindowProcStatic(HWND _hwnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
{
	cWindow* pWindow = nullptr; 

	if (_uMsg == WM_NCCREATE)
	{
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(_lParam); 
		pWindow = reinterpret_cast<cWindow*>(pCreate->lpCreateParams);
		SetWindowLongPtr(_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));

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
			PostQuitMessage(0);
			return 0; 
		}

		case WM_CLOSE:
		{
			DestroyWindow(hwnd); 
		}
	}

	return DefWindowProc(hwnd, _uMsg, _wParam, _lParam);
}

// --------------------------------------------------------------------------------------------------------------------------
