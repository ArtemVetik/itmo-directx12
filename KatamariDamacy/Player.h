#pragma once

#include "RenderComponent.h"
#include "../Common/Camera.h"
#include "DefaultMaterial.h"
#include "Mesh.h"
#include "Transformable.h"
#include "KatamariObject.h"

class Player : public RenderComponent, public Transformable
{
public:
	Player(Camera* camera, DefaultMaterial* material, Mesh* mesh, std::vector<KatamariObject*> objects);
	void Build() override;
	void Update(const GameTimer& t, DirectX::XMMATRIX viewProj, std::vector<ShadowMapConstants> shadowConstants) override;
	void Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList, int cbOffset = 0) override;
	void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

	DirectX::XMMATRIX GetTransformMatrix() override;
private:
	DirectX::XMVECTOR mPosition;
	DirectX::XMVECTOR mLook;
	DirectX::XMVECTOR mRotation;
	DirectX::BoundingOrientedBox mCollision;

	std::vector<KatamariObject*> mConnectedObjects;
	std::vector<KatamariObject*> mFreeObjects;

	Camera* mCamera;
	DefaultMaterial* mMaterial;
	Mesh* mMesh;

	// Inherited via Transformable
	DirectX::XMVECTOR GetPosition() override;

	// Inherited via Transformable
	DirectX::XMVECTOR GetRotation() override;

	// Inherited via Transformable
	Transformable* GetParent() override;
};

