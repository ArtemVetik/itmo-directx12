#pragma once

#include <array>
#include <DirectXMath.h>
#include <WindowsX.h>
#include <DirectXColors.h>
#include "../Common/d3dUtil.h"
#include "../Common/UploadBuffer.h"
#include "RenderComponent.h"
#include "Shader.h"
#include "Mesh.h"

class PlayerRenderComponent : public RenderComponent
{
public:
	struct Input
	{
		char moveUp;
		char moveDown;
	};

	PlayerRenderComponent(Mesh* mesh, Shader* shader, DirectX::XMFLOAT2 startPosition, Input input);

	void Build() override;
	void Update(const GameTimer& gt, DirectX::XMFLOAT4X4 mProj) override;
	void Draw(const GameTimer& gt, ID3D12GraphicsCommandList* commandList) override;
	void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	void Reset();

	DirectX::BoundingBox GetBoundingBox() const;

private:
	DirectX::XMFLOAT2 mPosition;
	bool mMoveUp;
	bool mMoveDown;
	Input mInput;

	Mesh* mMesh;
	Shader* mShader;
};

