#include "CameraRootController.h"

CameraRootController::CameraRootController(PlanetsApp* app, std::vector<PlanetRoot*> roots)
{
	mApp = app;
	mRoots = roots;
}

void CameraRootController::Build()
{
}

void CameraRootController::Update(const GameTimer& t, DirectX::XMMATRIX viewProj)
{
}

void CameraRootController::Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList)
{
}

void CameraRootController::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam >= '1' && wParam <= '9')
		{
			int index = wParam - '1';

			if (mRoots.size() > index)
				mApp->SetCameraRoot(mRoots[index]);
		}
		else if (wParam == '0')
		{
			mApp->SetCameraRoot(nullptr);
		}
		break;
	}
}
