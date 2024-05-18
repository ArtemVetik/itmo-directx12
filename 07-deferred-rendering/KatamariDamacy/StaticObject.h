#pragma once
#include "RenderComponent.h"
#include "DefaultMaterial.h"
#include "KatamariApp.h"
#include "Mesh.h"

class StaticObject : public RenderComponent
{
public:
	StaticObject(Mesh* mesh, DefaultMaterial* material, KatamariApp* app, DirectX::XMMATRIX transform);

	void Build() override;
	void Update(const GameTimer& t, DirectX::XMMATRIX viewProj, std::vector<ShadowMapConstants> shadowConstants) override;
	void Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList, int cbOffset = 0) override;
	void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
	DirectX::XMMATRIX mTransform;
	Mesh* mMesh;
	DefaultMaterial* mMaterial;
	KatamariApp* mApp;
};

