#include "WorldGrid.h"

WorldGrid::WorldGrid(Mesh* mesh, DefaultMaterial* material)
{
	mMesh = mesh;
	mMaterial = material;
}

void WorldGrid::Build()
{

}

void WorldGrid::Update(const GameTimer& t, DirectX::XMMATRIX viewProj)
{
	ObjectConstants objConstants;
	DirectX::XMStoreFloat4x4(&objConstants.ViewProj, viewProj);
	mMaterial->CopyData(0, objConstants);
}

void WorldGrid::Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList)
{
	commandList->IASetVertexBuffers(0, 1, &mMesh->GetVertexBufferView());
	commandList->IASetIndexBuffer(&mMesh->GetIndexBufferView());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	mMaterial->SetRenderState();

	for (auto& arg : mMesh->GetDrawArgs())
	{
		auto aaa = arg.first;
		auto bbb = arg.second;
		commandList->DrawIndexedInstanced(
			arg.second.IndexCount,
			1, 0, 0, 0);
	}
}

void WorldGrid::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
}
