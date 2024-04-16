#pragma once
#include <d3d12.h>
#include <DirectXCollision.h>
#include "RenderComponent.h"
#include "PlayerRenderComponent.h"
#include "Shader.h"
#include "Mesh.h"

class BallRenderComponent : public RenderComponent
{
public:
	BallRenderComponent(Mesh* mesh, Shader* shader, std::vector<PlayerRenderComponent*> players);
	void Build() override;
	void Update(const GameTimer& t, DirectX::XMFLOAT4X4 mProj) override;
	void Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList) override;
	void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	void Reset();

	DirectX::XMFLOAT2 GetPosition() const;
	PlayerRenderComponent* LastMove() const;
private:
	DirectX::XMFLOAT2 mPosition;
	DirectX::XMVECTOR mDirection;
	int mSpeed;
	std::vector<PlayerRenderComponent*> mPlayers;
	std::vector<PlayerRenderComponent*> mCollisions;
	PlayerRenderComponent* mLastMove;

	Mesh* mMesh;
	Shader* mShader;

};

