#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include "Shader.h"
#include "../Common/GameTimer.h"

class RenderComponent
{
public:
	virtual ~RenderComponent() = default;
	virtual void Build() = 0;
	virtual void Update(const GameTimer &t, DirectX::XMMATRIX viewProj, std::vector<ShadowMapConstants> shadowConstants) = 0;
	virtual void Draw(const GameTimer &t, ID3D12GraphicsCommandList* commandList, int cbOffset = 0) = 0;
	virtual void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
};
