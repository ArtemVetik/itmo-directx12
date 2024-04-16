#include "SSQuad.h"
#include "../Common/GeometryGenerator.h"

SSQuad::SSQuad(Mesh* mesh, DefaultMaterial* material)
{
	mMesh = mesh;
	mMaterial = material;
}

void SSQuad::Build()
{
}

void SSQuad::Update(const GameTimer& t, DirectX::XMMATRIX viewProj, ShadowMapConstants shadowConstants)
{
	ObjectConstants c{};
	mMaterial->CopyData(0, c);
}

void SSQuad::Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList, int cbOffset)
{
	mMaterial->SetRenderState(0);

	commandList->IASetVertexBuffers(0, 1, &mMesh->GetVertexBufferView());
	commandList->IASetIndexBuffer(&mMesh->GetIndexBufferView());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& arg : mMesh->GetDrawArgs())
	{
		commandList->DrawIndexedInstanced(
			arg.second.IndexCount,
			1, arg.second.StartIndexLocation, arg.second.BaseVertexLocation, 0);

		break;
	}
}

void SSQuad::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
}
