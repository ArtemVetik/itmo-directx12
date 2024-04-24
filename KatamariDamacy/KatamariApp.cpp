#include "KatamariApp.h"

KatamariApp::KatamariApp(GameWindow* gameWindow)
	: D3DApp(gameWindow)
{
	mLastMousePos = {};
	mCamera.SetPosition({ 0, 0, -3 });
}

KatamariApp::~KatamariApp()
{
}

bool KatamariApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	md3dDevice->QueryInterface(IID_PPV_ARGS(&infoQueue));
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

	mSSComponents.clear();

	return true;
}

void KatamariApp::Handle(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	D3DApp::Handle(hwnd, msg, wParam, lParam);

	switch (msg)
	{
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		mLastMousePos.x = GET_X_LPARAM(lParam);
		mLastMousePos.y = GET_Y_LPARAM(lParam);
		break;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		if ((wParam & MK_LBUTTON) != 0)
		{
			// Make each pixel correspond to a quarter of a degree.
			float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(GET_X_LPARAM(lParam) - mLastMousePos.x));
			float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(GET_Y_LPARAM(lParam) - mLastMousePos.y));

			mCamera.Pitch(dy);
			mCamera.RotateY(dx);
		}

		mLastMousePos.x = GET_X_LPARAM(lParam);
		mLastMousePos.y = GET_Y_LPARAM(lParam);
		break;
	case WM_KEYDOWN:
		if (wParam == 'P')
		{
			mCamera.TogglePerspective();
			Resize();
		}
		break;
	}

	for (auto& component : mComponents)
		component->HandleMessage(hwnd, msg, wParam, lParam);
}

void KatamariApp::InitShadowMap()
{
	mShadowShader = new Shader(md3dDevice.Get(), mCommandList.Get(), L"Shaders\\Shadows.hlsl");
	mShadowShader->Initialize();

	mShadowMaps.push_back(std::make_unique<ShadowMap>(md3dDevice.Get(), mShadowShader, 2048, 2048));
	mShadowMaps.push_back(std::make_unique<ShadowMap>(md3dDevice.Get(), mShadowShader, 1024, 1024));
	mShadowMaps.push_back(std::make_unique<ShadowMap>(md3dDevice.Get(), mShadowShader, 512, 512));
	mShadowMaps.push_back(std::make_unique<ShadowMap>(md3dDevice.Get(), mShadowShader, 256, 256));

	mShadowMaps[0]->Initialize(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), 0);
	mShadowMaps[1]->Initialize(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), 1);
	mShadowMaps[2]->Initialize(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), 2);
	mShadowMaps[3]->Initialize(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), 3);
}

void KatamariApp::AddComponent(RenderComponent* component)
{
	mComponents.push_back(component);
}

void KatamariApp::AddSSComponent(RenderComponent* component)
{
	mSSComponents.push_back(component);
}

void KatamariApp::AddLightQuadComponent(RenderComponent* component)
{
	mLightQuad = component;
}

void KatamariApp::RemoveComponent(RenderComponent* component)
{
	auto it = std::find(mComponents.begin(), mComponents.end(), component);

	if (it != mComponents.end())
		mComponents.erase(it);
}

Camera* KatamariApp::GetMainCamera()
{
	return &mCamera;
}

std::vector<ShadowMap*> KatamariApp::GetShadowMap()
{
	std::vector<ShadowMap*> maps;

	for (auto& m : mShadowMaps)
		maps.push_back(m.get());

	return maps;
}

void KatamariApp::Resize()
{
	D3DApp::Resize();

	mCamera.SetLens(0.25f * MathHelper::Pi, mGameWindow->GetAspectRatio(), 0.1f, 1000.0f);
}

void KatamariApp::OnUpdate(const GameTimer& gt)
{
	mCamera.UpdateViewMatrix();

	for (auto& map : mShadowMaps)
		map->Update(mCamera);

	DirectX::XMMATRIX view = mCamera.GetView();
	DirectX::XMMATRIX proj = mCamera.GetProj();
	DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);

	std::vector<ShadowMapConstants> shadowConstants;
	for (auto& map : mShadowMaps)
		shadowConstants.push_back(map->GetConstants());

	for (auto& component : mComponents)
		component->Update(gt, DirectX::XMMatrixTranspose(viewProj), shadowConstants);

	for (auto& component : mSSComponents)
		component->Update(gt, DirectX::XMMatrixTranspose(viewProj), shadowConstants);

	mLightQuad->Update(gt, DirectX::XMMatrixTranspose(viewProj), shadowConstants);
}

void KatamariApp::OnDraw(const GameTimer& gt)
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	for (auto &map : mShadowMaps)
		map->Draw(gt, mCommandList.Get(), mComponents);

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::Black, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDesc(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < RTVNum; i++) {
		mCommandList->ClearRenderTargetView(rtvDesc, DirectX::Colors::Black, 0, nullptr);
		rtvDesc.Offset(1, mRtvDescriptorSize);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDesc2(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(RTVNum, &rtvDesc2, true, &DepthStencilView());

	for (auto& component : mComponents)
		component->Draw(gt, mCommandList.Get());

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, nullptr);

	for (int i = 0; i < RTVNum; i++)
	{
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRtvTexture[i].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	// draw ss quad
	mLightQuad->Draw(mTimer, mCommandList.Get());

	for (auto& component : mSSComponents)
		component->Draw(gt, mCommandList.Get());

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrentBackBuffer = (mCurrentBackBuffer + 1) % SwapChainCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}