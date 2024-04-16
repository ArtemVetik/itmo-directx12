#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include "../Common/GameTimer.h"

class RenderComponent
{
public:
	virtual void Build() = 0;
	virtual void Update(const GameTimer &t, DirectX::XMFLOAT4X4 mProj) = 0;
	virtual void Draw(const GameTimer &t) = 0;
	virtual void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
};

