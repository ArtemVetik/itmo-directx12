#pragma once

#include <array>
#include <DirectXMath.h>
#include <WindowsX.h>
#include <DirectXColors.h>
#include "../Common/d3dUtil.h"
#include "../Common/UploadBuffer.h"
#include "RenderComponent.h"
#include "Shader.h"

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
};

class BoxRenderComponent : public RenderComponent
{
public:
	BoxRenderComponent(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Shader* shader, float shift = 0);

	void Build() override;
	void Update(const GameTimer& gt, DirectX::XMFLOAT4X4 mProj) override;
	void Draw(const GameTimer& gt) override;
	void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
	void BuildGeometry();

	void OnMouseDown(HWND hwnd, WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

	float mShift = 0;
	float mTheta = 1.5f * DirectX::XM_PI;
	float mPhi = DirectX::XM_PIDIV4;
	float mRadius = 5.0f;
	POINT mLastMousePos;

	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
	Shader* mShader;
	ID3D12Device* mDevice;
	ID3D12GraphicsCommandList* mCommandList;
};

