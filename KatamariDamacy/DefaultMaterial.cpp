#include "DefaultMaterial.h"

DefaultMaterial::DefaultMaterial(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Shader* shader)
{
    mDevice = device;
    mCommandList = commandList;
    mShader = shader;
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
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc {};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

void DefaultMaterial::BuildShaderResources()
{
    mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(mDevice, 1, true);
	
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

    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void DefaultMaterial::SetRenderState()
{
    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mShader->GetRootSignature());

    mCommandList->SetPipelineState(mPSO.Get());

    auto mCbvSrvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    tex.Offset(0, mCbvSrvDescriptorSize);

    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = mObjectCB->Resource()->GetGPUVirtualAddress() + 0 * objCBByteSize;

    mCommandList->SetGraphicsRootDescriptorTable(0, tex);
    mCommandList->SetGraphicsRootConstantBufferView(1, objCBAddress);
}

void DefaultMaterial::CopyData(int elementIndex, const ObjectConstants& data)
{
    mObjectCB->CopyData(elementIndex, data);
}
