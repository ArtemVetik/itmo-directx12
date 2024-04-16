#pragma once

#include <d3d12.h>
#include "../Common/d3dUtil.h"
#include "../Common/UploadBuffer.h"
#include "../Common/Constants.h"

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 AmbientLight = { 0.6f, 0.6f, 0.6f, 1.0f };
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float Roughness = 0.2f;
	DirectX::XMFLOAT3 FresnelR0 = { 0.05f, 0.05f, 0.05f };
	float gEmpty = 0.0f;
	Light Lights[MaxLights];
};

class Shader
{
public:
	Shader(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

	void Initialize();
	void BuildShadersAndInputLayout();
	void BuildRootSignature();

	D3D12_INPUT_LAYOUT_DESC GetInputLayout() const;
	ID3D12RootSignature* GetRootSignature() const;
	D3D12_SHADER_BYTECODE GetVS() const;
	D3D12_SHADER_BYTECODE GetPS() const;

private:
	ID3D12Device* mDevice;
	ID3D12GraphicsCommandList* mCommandList;
	Microsoft::WRL::ComPtr<ID3DBlob> mvsByteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mpsByteCode = nullptr;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ID3D12InfoQueue* mInfoQueue;
};

