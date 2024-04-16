#include "WorldGrid.h"

WorldGrid::WorldGrid(Mesh* mesh, DefaultMaterial* material)
{
	mMesh = mesh;
	mMaterial = material;
}

void WorldGrid::Build()
{

}

void WorldGrid::Update(const GameTimer& t, DirectX::XMMATRIX viewProj, std::vector<ShadowMapConstants> shadowConstants)
{
	ObjectConstants objConstants;
	DirectX::XMStoreFloat4x4(&objConstants.ViewProj, viewProj);
	DirectX::XMStoreFloat4x4(&objConstants.ShadowTransform[0], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&shadowConstants[0].ShadowTransform)));
	DirectX::XMStoreFloat4x4(&objConstants.ShadowTransform[1], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&shadowConstants[1].ShadowTransform)));
	mMaterial->CopyData(0, objConstants);

	for (size_t i = 0; i < shadowConstants.size(); i++)
	{
		XMStoreFloat4x4(&objConstants.ViewProj, shadowConstants[i].ViewProj);
		objConstants.EyePosW = shadowConstants[i].EyePosW;

		mMaterial->CopyData(i + 1, objConstants);
	}
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
