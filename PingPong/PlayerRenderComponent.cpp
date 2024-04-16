#include "PlayerRenderComponent.h"

PlayerRenderComponent::PlayerRenderComponent(Mesh* mesh, Shader* shader, DirectX::XMFLOAT2 startPosition, Input input)
{
	mMesh = mesh;
	mShader = shader;
	mPosition = startPosition;
	mInput = input;
	mMoveUp = mMoveDown = false;
}

void PlayerRenderComponent::Build()
{

}

void PlayerRenderComponent::Update(const GameTimer& gt, DirectX::XMFLOAT4X4 mProj)
{
	float delta = 0.0f;
	if (mMoveUp) delta += 1.0f;
	if (mMoveDown) delta -= 1.0f;

	mPosition.y += delta * gt.DeltaTime();
	ObjectConstants objConstants;
	objConstants.Position = { mPosition.x, mPosition.y, 0.0f, 0.0f };
	mShader->CbCopyData(0, objConstants);
}

void PlayerRenderComponent::Draw(const GameTimer& gt, ID3D12GraphicsCommandList* commandList)
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

void PlayerRenderComponent::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == mInput.moveDown)
			mMoveDown = true;
		else if (wParam == mInput.moveUp)
			mMoveUp = true;
		break;
	case WM_KEYUP:
		if (wParam == mInput.moveDown)
			mMoveDown = false;
		if (wParam == mInput.moveUp)
			mMoveUp = false;
		break;
	}
}

void PlayerRenderComponent::Reset()
{
	mPosition.y = 0.0f;
}

DirectX::BoundingBox PlayerRenderComponent::GetBoundingBox() const
{
	return DirectX::BoundingBox(
		{ mPosition.x, mPosition.y, 0.0f },
		{ 0.05f, 0.1f, 0.0f }
	);
}
