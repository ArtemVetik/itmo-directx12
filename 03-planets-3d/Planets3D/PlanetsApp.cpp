#include "PlanetsApp.h"

PlanetsApp::PlanetsApp(GameWindow* gameWindow)
	: D3DApp(gameWindow)
{
	mLastMousePos = {};
	mCameraRoot = nullptr;
	mCamera.SetPosition({ 0, 0, -3 });
}

PlanetsApp::~PlanetsApp()
{
}

bool PlanetsApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	return true;
}

void PlanetsApp::Handle(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
		if ((wParam & MK_LBUTTON) != 0 && mCameraRoot == nullptr)
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

void PlanetsApp::BuildMesh(Mesh* mesh)
{
	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mesh->Build();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();
}

void PlanetsApp::AddComponent(RenderComponent* component)
{
	mComponents.push_back(component);
}

void PlanetsApp::RemoveComponent(RenderComponent* component)
{
	auto it = std::find(mComponents.begin(), mComponents.end(), component);

	if (it != mComponents.end())
		mComponents.erase(it);
}

void PlanetsApp::SetCameraRoot(PlanetRoot* root)
{
	mCameraRoot = root;
}

void PlanetsApp::Resize()
{
	D3DApp::Resize();

	mCamera.SetLens(0.25f * MathHelper::Pi, mGameWindow->GetAspectRatio(), 0.1f, 1000.0f);
}

void PlanetsApp::OnUpdate(const GameTimer& gt)
{
	if (mCameraRoot)
	{
		mCamera.SetPosition(mCameraRoot->GetCurrentPosition());

		DirectX::XMVECTOR quatInverse = DirectX::XMQuaternionInverse(mCameraRoot->GetCurrentQuaternionRotation());
		DirectX::XMVECTOR forward = DirectX::XMQuaternionMultiply(quatInverse, { 0.0f, 0.0f, 1.0f });
		DirectX::XMVECTOR target = DirectX::XMVectorAdd(mCameraRoot->GetCurrentPositionVector(), forward);

		DirectX::XMFLOAT3 targetPos;
		DirectX::XMStoreFloat3(&targetPos, target);

		mCamera.LookAt(
			mCameraRoot->GetCurrentPosition(),
			targetPos,
			{ 0.0f, 1.0f, 0.0f }
		);
	}
	else
	{
		const float dt = mTimer.DeltaTime();

		if (GetAsyncKeyState('W') & 0x8000)
			mCamera.Walk(10.0f * dt);

		if (GetAsyncKeyState('S') & 0x8000)
			mCamera.Walk(-10.0f * dt);

		if (GetAsyncKeyState('A') & 0x8000)
			mCamera.Strafe(-10.0f * dt);

		if (GetAsyncKeyState('D') & 0x8000)
			mCamera.Strafe(10.0f * dt);

		if (GetAsyncKeyState('E') & 0x8000)
			mCamera.VerticalMove(10.0f * dt);

		if (GetAsyncKeyState('Q') & 0x8000)
			mCamera.VerticalMove(-10.0f * dt);
	}

	mCamera.UpdateViewMatrix();

	DirectX::XMMATRIX view = mCamera.GetView();
	DirectX::XMMATRIX proj = mCamera.GetProj();
	DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);

	for (auto& component : mComponents)
		component->Update(gt, DirectX::XMMatrixTranspose(viewProj));
}

void PlanetsApp::OnDraw(const GameTimer& gt)
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	for (auto& component : mComponents)
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
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}