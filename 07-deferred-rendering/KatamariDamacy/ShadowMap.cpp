#include "ShadowMap.h"
#include "../Common/Constants.h"
#include "RenderComponent.h"
#include "../Common/Camera.h"
#include <iostream>

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> ShadowMap::ShadowMapsHeap = nullptr;

ShadowMap::ShadowMap(ID3D12Device* device, Shader* shader, UINT width, UINT height)
{
	mDevice = device;
	mShader = shader;
	mWidth = width;
	mHeight = height;

	mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	mScissorRect = { 0, 0, (int)width, (int)height };

	BuildResource();
}

UINT ShadowMap::Width()const
{
	return mWidth;
}

UINT ShadowMap::Height()const
{
	return mHeight;
}

ID3D12Resource* ShadowMap::Resource()
{
	return mShadowMap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMap::Srv()const
{
	return mhGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::Dsv()const
{
	return mhCpuDsv;
}

D3D12_VIEWPORT ShadowMap::Viewport()const
{
	return mViewport;
}

D3D12_RECT ShadowMap::ScissorRect()const
{
	return mScissorRect;
}

ShadowMapConstants ShadowMap::GetConstants()
{
	DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&mLightView);
	DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&mLightProj);

	DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);

	ShadowMapConstants mShadowPassCB{};

	mShadowPassCB.ViewProj = XMMatrixTranspose(viewProj);
	mShadowPassCB.EyePosW = mLightPosW;
	mShadowPassCB.ShadowTransform = mShadowTransform;

	return mShadowPassCB;
}

void ShadowMap::Initialize(D3D12_CPU_DESCRIPTOR_HANDLE dsvStart, int index)
{
	mIndex = index;

	if (ShadowMapsHeap == nullptr)
	{
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
		cbvHeapDesc.NumDescriptors = 4;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(mDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&ShadowMapsHeap)));
	}

	auto mCbvSrvUavDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto mDsvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	auto a = ShadowMapsHeap->GetCPUDescriptorHandleForHeapStart();
	auto b = ShadowMapsHeap->GetGPUDescriptorHandleForHeapStart();

	BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE(a, index, mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(b, index, mCbvSrvUavDescriptorSize),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvStart, index + 1, mDsvDescriptorSize));

	D3D12_RASTERIZER_DESC rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterizer.DepthBias = 100000;
	rasterizer.DepthBiasClamp = 0.0f;
	rasterizer.SlopeScaledDepthBias = 1.0f;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = mShader->GetInputLayout();
	psoDesc.RasterizerState = rasterizer;
	psoDesc.pRootSignature = mShader->GetRootSignature();
	psoDesc.VS = mShader->GetVS();
	psoDesc.PS = mShader->GetPS();
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 0;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	psoDesc.SampleDesc.Count = Constants::m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = Constants::m4xMsaaState ? (Constants::m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = Constants::mDepthStencilFormat;

	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void ShadowMap::Update(Camera camera)
{
	DirectX::XMVECTOR lightDir = { 0.57735f, -0.57735f, 0.57735f };//{ 0.57735f, -0.57735f, 0.57735f };
	DirectX::XMVECTOR lightPos = DirectX::XMVectorMultiply(lightDir, { -51, -51, -51 });;// { 0.0f * -51, -0.70710f * -51, 0.70710f * -51 };
	DirectX::XMVECTOR targetPos = { 0, 0, 0 };
	DirectX::XMVECTOR lightUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX lightView = DirectX::XMMatrixLookAtLH(lightPos, targetPos, lightUp);
	DirectX::XMStoreFloat4x4(&mLightView, lightView);

	DirectX::XMVECTOR pos;
	DirectX::XMFLOAT3 minPos;
	DirectX::XMFLOAT3 maxPos;

	if (mIndex == 0)
		CalcCascadePos(camera, 1.0f, 15.0f, pos, minPos, maxPos);
	if (mIndex == 1)
		CalcCascadePos(camera, 15.0f, 30.0f, pos, minPos, maxPos);
	if (mIndex == 2)
		CalcCascadePos(camera, 30.0f, 45.0f, pos, minPos, maxPos);
	if (mIndex == 3)
		CalcCascadePos(camera, 45.0f, 60.0f, pos, minPos, maxPos);

	mLightPos = pos;
	mTargetPos = DirectX::XMVectorMultiplyAdd(lightDir, { 1 ,1, 1 }, pos);

	//DirectX::XMMATRIX lightView = DirectX::XMMatrixLookAtLH(mLightPos, mTargetPos, lightUp);
	//DirectX::XMStoreFloat4x4(&mLightView, lightView);

	DirectX::XMStoreFloat3(&mLightPosW, mLightPos);

	// Transform bounding sphere to light space.
	DirectX::XMFLOAT3 sphereCenterLS;
	DirectX::XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(mTargetPos, lightView));

	// Ortho frustum in light space encloses scene.
	float l = minPos.x;// sphereCenterLS.x - size;
	float b = minPos.y;// sphereCenterLS.y - size;
	float n = minPos.z - 10;// sphereCenterLS.z - size;
	float r = maxPos.x;// sphereCenterLS.x + size;
	float t = maxPos.y;//sphereCenterLS.y + size;
	float f = maxPos.z + 10;//sphereCenterLS.z + size + 100;

	mLightNearZ = n;
	mLightFarZ = f;
	DirectX::XMMATRIX lightProj = DirectX::XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	DirectX::XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	DirectX::XMMATRIX S = lightView * lightProj * T;
	DirectX::XMStoreFloat4x4(&mLightProj, lightProj);
	DirectX::XMStoreFloat4x4(&mShadowTransform, S);
}

void ShadowMap::Draw(const GameTimer& gt, ID3D12GraphicsCommandList* mCommandList, std::vector<RenderComponent*> components)
{
	mCommandList->RSSetViewports(1, &Viewport());
	mCommandList->RSSetScissorRects(1, &ScissorRect());

	// Change to DEPTH_WRITE.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Resource(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearDepthStencilView(Dsv(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set null render target because we are only going to draw to
	// depth buffer.  Setting a null render target will disable color writes.
	// Note the active PSO also must specify a render target count of 0.
	mCommandList->OMSetRenderTargets(0, nullptr, false, &Dsv());

	// Bind the pass constant buffer for the shadow map pass.

	mCommandList->SetPipelineState(mPSO.Get());

	for (auto& component : components)
		component->Draw(gt, mCommandList, mIndex + 1);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Resource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void ShadowMap::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
	// Save references to the descriptors. 
	mhCpuSrv = hCpuSrv;
	mhGpuSrv = hGpuSrv;
	mhCpuDsv = hCpuDsv;

	//  Create the descriptors
	BuildDescriptors();
}

void ShadowMap::OnResize(UINT newWidth, UINT newHeight)
{
	if ((mWidth != newWidth) || (mHeight != newHeight))
	{
		mWidth = newWidth;
		mHeight = newHeight;

		BuildResource();

		// New resource, so we need new descriptors to that resource.
		BuildDescriptors();
	}
}

void ShadowMap::BuildDescriptors()
{
	// Create SRV to resource so we can sample the shadow map in a shader program.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	mDevice->CreateShaderResourceView(mShadowMap.Get(), &srvDesc, mhCpuSrv);

	// Create DSV to resource so we can render to the shadow map.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	mDevice->CreateDepthStencilView(mShadowMap.Get(), &dsvDesc, mhCpuDsv);
}

void ShadowMap::BuildResource()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mShadowMap)));
}

void ShadowMap::CalcCascadePos(Camera camera, float nearDist, float farDist, DirectX::XMVECTOR& centerPosOut, DirectX::XMFLOAT3& minPos, DirectX::XMFLOAT3& maxPos)
{
	float nx1 = nearDist * tan(camera.GetFovX() / 2.0);
	float fx1 = farDist * tan(camera.GetFovX() / 2.0);
	float ny1 = nearDist * tan(camera.GetFovY() / 2.0);
	float fy1 = farDist * tan(camera.GetFovY() / 2.0);

	DirectX::XMVECTOR fPoses[8] =
	{
		{ +nx1, +ny1, nearDist, 1 },
		{ -nx1, +ny1, nearDist, 1 },
		{ +nx1, -ny1, nearDist, 1 },
		{ -nx1, -ny1, nearDist, 1 },
		{ +fx1, +fy1, farDist,  1 },
		{ -fx1, +fy1, farDist,  1 },
		{ +fx1, -fy1, farDist,  1 },
		{ -fx1, -fy1, farDist,  1 },
	};

	DirectX::XMMATRIX lightView = DirectX::XMLoadFloat4x4(&mLightView);

	for (size_t i = 0; i < 8; i++)
	{
		fPoses[i] = DirectX::XMVector3TransformCoord(fPoses[i], DirectX::XMMatrixInverse(nullptr, camera.GetView()));
		fPoses[i] = DirectX::XMVector3TransformCoord(fPoses[i], lightView);
	}

	float minX = FLT_MAX;
	float minY = FLT_MAX;
	float minZ = FLT_MAX;
	float maxX = -FLT_MAX;
	float maxY = -FLT_MAX;
	float maxZ = -FLT_MAX;

	for (size_t i = 0; i < 8; i++)
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMStoreFloat3(&pos, fPoses[i]);

		//float x = DirectX::XMVectorGetX(fPoses[i]);
		//float y = DirectX::XMVectorGetY(fPoses[i]);
		//float z = DirectX::XMVectorGetZ(fPoses[i]);

		float x = pos.x;
		float y = pos.y;
		float z = pos.z;

		if (x < minX) minX = x;
		if (x > maxX) maxX = x;
		if (y < minY) minY = y;
		if (y > maxY) maxY = y;
		if (z < minZ) minZ = z;
		if (z > maxZ) maxZ = z;
	}

	float xLen = maxX - minX;
	float yLen = maxY - minY;
	float zLen = maxZ - minZ;

	DirectX::XMVECTOR centerPos = { minX + xLen / 2.0f, minY + yLen / 2.0f, 0 };
	centerPosOut = DirectX::XMVector3TransformCoord(centerPos, DirectX::XMMatrixInverse(nullptr, lightView));

	minPos = { minX, minY, minZ };
	maxPos = { maxX, maxY, maxZ };
}
