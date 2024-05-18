#include "DefaultMaterial.h"

DefaultMaterial::DefaultMaterial(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Shader* shader, KatamariApp* app)
{
	mDevice = device;
	mCommandList = commandList;
	mShader = shader;
	mApp = app;
}

void DefaultMaterial::Initialize(std::string texturePath)
{
	LoadTexture(texturePath);
	BuildDescriptorHeap();
	BuildShaderResources();
	BuildPSO();
}

void DefaultMaterial::LoadTexture(std::string texturePath)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wTexturePath = converter.from_bytes(texturePath);

	mTexture = std::make_unique<Texture>();
	mTexture->Name = texturePath;
	mTexture->Filename = wTexturePath;

	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mDevice,
		mCommandList, mTexture->Filename.c_str(),
		mTexture->Resource, mTexture->UploadHeap));
}

void DefaultMaterial::BuildDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
	cbvHeapDesc.NumDescriptors = 3;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

void DefaultMaterial::BuildShaderResources()
{
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(mDevice, 5, true);

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mCbvHeap->GetCPUDescriptorHandleForHeapStart());

	auto textureResource = mTexture->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureResource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = textureResource->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	mDevice->CreateShaderResourceView(textureResource.Get(), &srvDesc, hDescriptor);
}
#include <iostream>
void DefaultMaterial::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = mShader->GetInputLayout();
	psoDesc.pRootSignature = mShader->GetRootSignature();
	psoDesc.VS = mShader->GetVS();
	psoDesc.PS = mShader->GetPS();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = Constants::mBackBufferFormat;
	psoDesc.SampleDesc.Count = Constants::m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = Constants::m4xMsaaState ? (Constants::m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = Constants::mDepthStencilFormat;

	//ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
	//return;
	HRESULT hr = mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO));

	if (FAILED(hr))
	{
		UINT64 numMessages = mApp->infoQueue->GetNumStoredMessages();

		for (UINT64 i = 0; i < numMessages; ++i)
		{
			SIZE_T messageLength;
			mApp->infoQueue->GetMessage(i, nullptr, &messageLength);

			D3D12_MESSAGE* pMessage = (D3D12_MESSAGE*)malloc(messageLength);
			mApp->infoQueue->GetMessage(i, pMessage, &messageLength);

			// Handle or log the error message
			// e.g., printf("Direct3D 12 Error: %s\n", pMessage->pDescription);
			std::cout << "D3D ERROR::: " << pMessage->pDescription << "\n";
			free(pMessage);
		}

		// Clear the stored messages after processing
		mApp->infoQueue->ClearStoredMessages();
		throw 12321;
	}
	else
	{
		std::cout << "Correct!\n";
	}
}

void DefaultMaterial::SetRenderState(int cbOffset)
{
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mShader->GetRootSignature());

	if (cbOffset == 0)
		mCommandList->SetPipelineState(mPSO.Get());

	auto mCbvSrvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->SetGraphicsRootDescriptorTable(0, tex);

	auto shadowMaps = mApp->GetShadowMap();

	ID3D12DescriptorHeap* descriptorHeaps2[] = { ShadowMap::ShadowMapsHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps2), descriptorHeaps2);

	CD3DX12_GPU_DESCRIPTOR_HANDLE shad(ShadowMap::ShadowMapsHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->SetGraphicsRootDescriptorTable(2, shad);

	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = mObjectCB->Resource()->GetGPUVirtualAddress() + cbOffset * objCBByteSize;

	mCommandList->SetGraphicsRootConstantBufferView(1, objCBAddress);
}

void DefaultMaterial::CopyData(int elementIndex, const ObjectConstants& data)
{
	mObjectCB->CopyData(elementIndex, data);
}
