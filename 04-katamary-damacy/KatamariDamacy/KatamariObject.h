#pragma once
#include <d3d12.h>

#include "DefaultMaterial.h"
#include "RenderComponent.h"
#include "Mesh.h"
#include "Shader.h"
#include "Transformable.h"
#include "KatamariApp.h"

struct KatamariObjectSettings
{
	DirectX::XMVECTOR Position = { 0.0f, 0.0f,0.0f };
	DirectX::XMVECTOR Scale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMVECTOR Rotation = DirectX::XMQuaternionIdentity();
};

class KatamariObject : public RenderComponent, public Transformable
{
public:
	KatamariObject(Mesh* mesh, DefaultMaterial* material, KatamariObjectSettings settings, KatamariApp* app);
	void Build() override;
	void Update(const GameTimer& t, DirectX::XMMATRIX viewProj) override;
	void Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList) override;
	void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

	DirectX::BoundingOrientedBox GetBoundingBox() const;
	void SetParent(Transformable* parent);

	DirectX::XMMATRIX GetTransformMatrix() override;
private:
	DirectX::XMVECTOR mPosition;
	DirectX::XMVECTOR mRotation;
	DirectX::BoundingOrientedBox mStartBoundingBox;
	DirectX::BoundingOrientedBox mCollision;

	KatamariObjectSettings mSettings;

	Mesh* mMesh;
	DefaultMaterial* mMaterial;
	Transformable* mParent;
	KatamariApp* mApp;

	// Inherited via Transformable
	DirectX::XMVECTOR GetPosition() override;

	// Inherited via Transformable
	DirectX::XMVECTOR GetRotation() override;

	// Inherited via Transformable
	Transformable* GetParent() override;
};

