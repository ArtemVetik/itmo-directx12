#pragma once

#include <windows.h>
#include <WindowsX.h>
#include <string>
#include <cassert>

class WindowMessageHandler
{
public:
	virtual void Handle(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
};

class GameWindow
{
public:
	GameWindow(HINSTANCE hInstance);
	bool Initialize();
	MSG ExecuteMsg();

	int GetClientWidth() const;
	int GetClientHeight() const;
	int IsPaused() const;
	HWND GetMainWnd() const;
	float GetAspectRatio() const;
	
	void AddHandler(WindowMessageHandler* handler);
	void RemoveHandler(WindowMessageHandler* handler);
	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static GameWindow* GetInstance();
private:
	static constexpr int maxHandlerCount = 16;

	HINSTANCE mhAppInst = nullptr; // application instance handle
	HWND      mhMainWnd = nullptr; // main window handle
	bool      mAppPaused = false;  // is the application paused?
	bool      mMinimized = false;  // is the application minimized?
	bool      mMaximized = false;  // is the application maximized?
	bool      mResizing = false;   // are the resize bars being dragged?
	bool      mFullscreenState = false;// fullscreen enabled
	int mClientWidth = 800;
	int mClientHeight = 600;
	std::wstring mMainWndCaption = L"d3d App";
	WindowMessageHandler* mHandlers[maxHandlerCount];
};