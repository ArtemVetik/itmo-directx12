#include "WorldGrid.h"

WorldGrid::WorldGrid(Mesh* mesh, DefaultMaterial* material)
{
	mMesh = mesh;
	mMaterial = material;
}

void WorldGrid::Build()
{

}

void WorldGrid::Update(const GameTimer& t, DirectX::XMMATRIX viewProj, ShadowMapConstants shadowConstants)
{
	ObjectConstants objConstants;
	DirectX::XMStoreFloat4x4(&objConstants.ViewProj, viewProj);
	DirectX::XMMATRIX shadowTransform = DirectX::XMLoadFloat4x4(&shadowConstants.ShadowTransform);
	DirectX::XMStoreFloat4x4(&objConstants.ShadowTransform, DirectX::XMMatrixTranspose(shadowTransform));
	mMaterial->CopyData(0, objConstants);

	XMStoreFloat4x4(&objConstants.ViewProj, shadowConstants.ViewProj);
	objConstants.EyePosW = shadowConstants.EyePosW;

	mMaterial->CopyData(1, objConstants);
}

void WorldGrid::Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList, int cbOffset)
{
	commandList->IASetVertexBuffers(0, 1, &mMesh->GetVertexBufferView());
	commandList->IASetIndexBuffer(&mMesh->GetIndexBufferView());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	mMaterial->SetRenderState(cbOffset);

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
