#pragma once
#include <d3d12.h>
#include "RenderComponent.h"
#include "Mesh.h"
#include "Shader.h"

class WorldGrid : public RenderComponent
{
public:
	WorldGrid(Mesh* mesh, Shader* shader);
	void Build() override;
	void Update(const GameTimer& t, DirectX::XMMATRIX viewProj) override;
	void Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList) override;
	void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
	Mesh* mMesh;
	Shader* mShader;
};

