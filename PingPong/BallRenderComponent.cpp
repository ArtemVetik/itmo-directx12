#include "BallRenderComponent.h"

BallRenderComponent::BallRenderComponent(Mesh* mesh, Shader* shader, std::vector<PlayerRenderComponent*> players)
{
	mMesh = mesh;
	mShader = shader;
	mPlayers = players;
	Reset();
}

void BallRenderComponent::Build()
{
	
}

void BallRenderComponent::Update(const GameTimer& t, DirectX::XMFLOAT4X4 mProj)
{
	std::vector<PlayerRenderComponent*> tempCollisions;
	DirectX::BoundingSphere sphere({ mPosition.x, mPosition.y, 0.0f }, 0.05f);

	for (size_t i = 0; i < mPlayers.size(); i++)
	{
		DirectX::BoundingBox box = mPlayers[i]->GetBoundingBox();
		if (sphere.Intersects(box))
		{
			tempCollisions.push_back(mPlayers[i]);

			if (std::find(mCollisions.begin(), mCollisions.end(), mPlayers[i]) != mCollisions.end())
				continue;

			mCollisions.push_back(mPlayers[i]);

			DirectX::XMVECTOR dir = DirectX::XMVectorSubtract({mPosition.x, mPosition.y}, {box.Center.x, box.Center.y});
			dir = DirectX::XMVector2Normalize(dir);

			mDirection = dir;
			mSpeed += 5;
			mLastMove = mPlayers[i];
			break;
		}
	}

	for (int i = mCollisions.size() - 1; i >= 0; i--)
	{
		if (std::find(tempCollisions.begin(), tempCollisions.end(), mCollisions[i]) == tempCollisions.end())
			mCollisions.erase(mCollisions.begin() + i);
	}

	mPosition.x += DirectX::XMVectorGetX(mDirection) * t.DeltaTime() * (mSpeed / 100.0f);
	mPosition.y += DirectX::XMVectorGetY(mDirection) * t.DeltaTime() * (mSpeed / 100.0f);

	if (abs(mPosition.y) >= 0.95f)
		mDirection = DirectX::XMVectorMultiply(mDirection, { 1.0f, -1.0f });
	
	ObjectConstants objConstants;
	objConstants.Position = { mPosition.x, mPosition.y, 0.0f, 0.0f };
	mShader->CbCopyData(0, objConstants);
}

void BallRenderComponent::Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList)
{
	mShader->SetRenderState(commandList);

	commandList->IASetVertexBuffers(0, 1, &mMesh->GetVertexBufferView());
	commandList->IASetIndexBuffer(&mMesh->GetIndexBufferView());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& arg : mMesh->GetDrawArgs())
	{
		commandList->DrawIndexedInstanced(
			arg.second.IndexCount,
			1, 0, 0, 0);
	}
}

void BallRenderComponent::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
}

void BallRenderComponent::Reset()
{
	mSpeed = 50;
	mDirection = { -1.0f, 0.0f };
	mPosition = {};
	mLastMove = nullptr;
}

DirectX::XMFLOAT2 BallRenderComponent::GetPosition() const
{
	return mPosition;
}

PlayerRenderComponent* BallRenderComponent::LastMove() const
{
	return mLastMove;
}
