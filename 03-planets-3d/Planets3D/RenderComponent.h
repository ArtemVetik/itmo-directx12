#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include "../Common/GameTimer.h"

class RenderComponent
{
public:
	virtual void Build() = 0;
	virtual void Update(const GameTimer &t, DirectX::XMMATRIX viewProj) = 0;
	virtual void Draw(const GameTimer &t, ID3D12GraphicsCommandList* commandList) = 0;
	virtual void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
};

