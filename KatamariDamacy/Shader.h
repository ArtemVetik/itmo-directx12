#pragma once

#include <d3d12.h>
#include "../Common/d3dUtil.h"
#include "../Common/UploadBuffer.h"
#include "../Common/Constants.h"

struct ShadowMapConstants
{
	DirectX::XMMATRIX ViewProj = {};
	DirectX::XMFLOAT4X4 ShadowTransform = MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
};

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewInv = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ProjInv = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ShadowTransform[4] = { MathHelper::Identity4x4(), MathHelper::Identity4x4(), MathHelper::Identity4x4(), MathHelper::Identity4x4() };
	DirectX::XMFLOAT4 DiffuseAlbedo = { 0.6f, 0.6f, 0.6f, 1.0f };
	DirectX::XMFLOAT4 AmbientLight = { 0.2f, 0.2f, 0.2f, 1.0f };
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	UINT ShadowIndex = 0;
	DirectX::XMFLOAT3 LookDir = { 0.0f, 0.0f, 0.0f };
	UINT Space0 = 0;
	DirectX::XMFLOAT3 FresnelR0 = { 0.015f, 0.015f, 0.015f };
	float Roughness = 0.2;
	DirectX::XMFLOAT4 CascadeDistances = { 15.0f, 30.0f, 45.0f, 60.0f };
	Light Lights[MaxLights]
	{
		 { { 0.5f, 0.5f, 0.5f }, 1.0f, { 0.57735f, -0.57735f, 0.57735f }, 10.0f, { }, 64.0f },
		 { { 1095.3f, 0.0f, 0.0f }, 1.0f, { -1.0f, 0.0f, 0.0f }, 15.0f, { 5, 8, 5 }, 64.0f },
		 { { 0.3f, 5.3f, 0.3f }, 1.0f, { -1.0f, 0.0f, 0.0f }, 15.0f, { 5, 8, -5 }, 64.0f },
		 { { 0.3f, 0.3f, 5.3f }, 1.0f, { -1.0f, 0.0f, 0.0f }, 15.0f, { -8, 8, 0 }, 64.0f },
		 //{ { 0.3f, 1.5f, 0.3f }, 1.0f, { 0.0f, -0.707f, -0.707f }, 15.0f, {5, 5, 0 }, 64.0f},
		 //{ { 0.3f, 0.3f, 1.5f }, 1.0f, { 0.0f, -0.707f, -0.707f }, 15.0f, { 0, 5, 5 }, 64.0f},
	};
};

class Shader
{
public:
	Shader(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::wstring shaderPath);

	void Initialize();
	void BuildShadersAndInputLayout();
	void BuildRootSignature();

	D3D12_INPUT_LAYOUT_DESC GetInputLayout() const;
	ID3D12RootSignature* GetRootSignature() const;
	D3D12_SHADER_BYTECODE GetVS() const;
	D3D12_SHADER_BYTECODE GetPS() const;

private:
	std::wstring mShaderPath;
	ID3D12Device* mDevice;
	ID3D12GraphicsCommandList* mCommandList;
	Microsoft::WRL::ComPtr<ID3DBlob> mvsByteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mpsByteCode = nullptr;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ID3D12InfoQueue* mInfoQueue;
};

