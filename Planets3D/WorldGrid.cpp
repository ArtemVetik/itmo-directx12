#include "WorldGrid.h"

WorldGrid::WorldGrid(Mesh* mesh, Shader* shader)
{
	mMesh = mesh;
	mShader = shader;
}

void WorldGrid::Build()
{

}

void WorldGrid::Update(const GameTimer& t, DirectX::XMMATRIX viewProj)
{
	ObjectConstants objConstants;
	objConstants.Position = { 0.0f, 0.0f, 0.0f, 0.0f };
	DirectX::XMStoreFloat4x4(&objConstants.WorldViewProj, viewProj);
	mShader->CbCopyData(0, objConstants);
}

void WorldGrid::Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList)
{
	mShader->SetRenderState(commandList);

	commandList->IASetVertexBuffers(0, 1, &mMesh->GetVertexBufferView());
	commandList->IASetIndexBuffer(&mMesh->GetIndexBufferView());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	for (auto& arg : mMesh->GetDrawArgs())
	{
		commandList->DrawIndexedInstanced(
			arg.second.IndexCount,
			1, 0, 0, 0);
	}
}

void WorldGrid::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
}
