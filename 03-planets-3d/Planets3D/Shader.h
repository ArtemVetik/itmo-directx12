#pragma once

#include <d3d12.h>
#include "../Common/d3dUtil.h"
#include "../Common/UploadBuffer.h"
#include "../Common/Constants.h"

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4 Position = { };
};

class Shader
{
public:
	Shader(ID3D12Device* device);

	void Initialize();
	void BuildShadersAndInputLayout();
	void BuildDescriptorHeap();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildPSO();

	void SetRenderState(ID3D12GraphicsCommandList* commandList);
	void CbCopyData(int elementIndex, const ObjectConstants& data);
private:
	ID3D12Device* mDevice;
	Microsoft::WRL::ComPtr<ID3DBlob> mvsByteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mpsByteCode = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
};

