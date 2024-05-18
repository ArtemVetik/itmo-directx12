#include "GameWindow.h"

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return GameWindow::GetInstance()->MsgProc(hwnd, msg, wParam, lParam);
}

GameWindow* mInstance = nullptr;

GameWindow::GameWindow(HINSTANCE hInstance)
	: mhAppInst(hInstance)
{
	assert(mInstance == nullptr);
	mInstance = this;
	
	for (int i = 0; i < maxHandlerCount; i++)
		mHandlers[i] = nullptr;
}

bool GameWindow::Initialize()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	/*mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);*/

	mhMainWnd = CreateWindowEx(WS_EX_APPWINDOW, L"MainWnd", mMainWndCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);

	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

LRESULT GameWindow::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
			mAppPaused = true;
		else
			mAppPaused = false;
		break;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (wParam == SIZE_MINIMIZED)
		{
			mAppPaused = true;
			mMinimized = true;
			mMaximized = false;
		}
		else if (wParam == SIZE_MAXIMIZED)
		{
			mAppPaused = false;
			mMinimized = false;
			mMaximized = true;
		}
		else if (wParam == SIZE_RESTORED)
		{
			// Restoring from minimized state?
			if (mMinimized)
			{
				mAppPaused = false;
				mMinimized = false;
			}

			// Restoring from maximized state?
			else if (mMaximized)
			{
				mAppPaused = false;
				mMaximized = false;
			}
		}
		break;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		break;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		break;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		break;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		/*else if ((int)wParam == VK_F2)
			Set4xMsaaState(!m4xMsaaState);*/

		break;
	}

	for (int i = 0; i < maxHandlerCount; i++)
		if (mHandlers[i])
			mHandlers[i]->Handle(hwnd, msg, wParam, lParam);

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

MSG GameWindow::ExecuteMsg()
{
	MSG msg = { 0 };

	if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg;
}

int GameWindow::GetClientWidth() const
{
	return mClientWidth;
}

int GameWindow::GetClientHeight() const
{
	return mClientHeight;
}

int GameWindow::IsPaused() const
{
	return mAppPaused;
}

HWND GameWindow::GetMainWnd() const
{
	return mhMainWnd;
}

float GameWindow::GetAspectRatio() const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

void GameWindow::AddHandler(WindowMessageHandler* handler)
{
	for (int i = 0; i < maxHandlerCount; i++)
	{
		if (mHandlers[i] == nullptr)
		{
			mHandlers[i] = handler;
			return;
		}
	}
}

void GameWindow::RemoveHandler(WindowMessageHandler* handler)
{
	for (int i = 0; i < maxHandlerCount; i++)
	{
		if (mHandlers[i] == handler)
		{
			mHandlers[i] = nullptr;
			return;
		}
	}
}

GameWindow* GameWindow::GetInstance()
{
	return mInstance;
}
