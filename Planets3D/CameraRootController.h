#pragma once
#include <d3d12.h>
#include "PlanetsApp.h"
#include "PlanetRenderComponent.h"

class CameraRootController : public RenderComponent
{
public:
	CameraRootController(PlanetsApp* app, std::vector<PlanetRoot*> roots);
	void Build() override;
	void Update(const GameTimer& t, DirectX::XMMATRIX viewProj) override;
	void Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList) override;
	void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
	PlanetsApp* mApp;
	std::vector<PlanetRoot*> mRoots;
};

