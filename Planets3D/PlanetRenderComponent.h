#pragma once
#include <d3d12.h>
#include "RenderComponent.h"
#include "Mesh.h"
#include "Shader.h"
#include "PlanetRoot.h"

struct PlanetSettings
{
	PlanetRoot* Root = nullptr;
	DirectX::XMVECTOR Scale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMVECTOR Rotation = DirectX::XMQuaternionIdentity();
	float OrbitAngle = 0.0f;
	float RotateRadius = 0.0f;
	float RotateAroundSpeed = 0.0f;
	float RotateSelfSpeed = 0.0f;
	float StartAngle = 0.0f;
};

class PlanetRenderComponent : public RenderComponent, public PlanetRoot
{
public:
	PlanetRenderComponent(Mesh* mesh, Shader* shader, PlanetSettings settings);
	void Build() override;
	void Update(const GameTimer& t, DirectX::XMMATRIX viewProj) override;
	void Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList) override;
	void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

	DirectX::XMFLOAT3 GetCurrentPosition() override;
	DirectX::XMVECTOR GetCurrentPositionVector() override;
	DirectX::XMVECTOR GetCurrentQuaternionRotation() override;

private:
	DirectX::XMVECTOR mPosition;
	DirectX::XMVECTOR mRotation;

	PlanetSettings mSettings;
	float mAroundAngle;
	float mSelfAngle;

	Mesh* mMesh;
	Shader* mShader;
};

