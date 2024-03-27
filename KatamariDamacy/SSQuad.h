#pragma once
#include "..\Common\d3dUtil.h"
#include "RenderComponent.h"
#include "DefaultMaterial.h"
#include "Mesh.h"

class SSQuad : public RenderComponent
{
public:
	SSQuad(Mesh* mesh, DefaultMaterial* material);
	void Build() override;
	void Update(const GameTimer& t, DirectX::XMMATRIX viewProj, ShadowMapConstants shadowConstants) override;
	void Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList, int cbOffset) override;
	void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
	Mesh* mMesh;
	DefaultMaterial* mMaterial;
};

