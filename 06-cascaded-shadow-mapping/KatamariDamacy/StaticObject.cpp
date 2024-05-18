#include "StaticObject.h"

StaticObject::StaticObject(Mesh* mesh, DefaultMaterial* material, KatamariApp* app, DirectX::XMMATRIX transform)
{
	mMesh = mesh;
	mMaterial = material;
	mApp = app;
	mTransform = transform;
}

void StaticObject::Build()
{

}

void StaticObject::Update(const GameTimer& t, DirectX::XMMATRIX viewProj, std::vector<ShadowMapConstants> shadowConstants)
{
	DirectX::XMMATRIX texTransform = XMLoadFloat4x4(&MathHelper::Identity4x4());

	ObjectConstants objConstants{};
	XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(mTransform));
	DirectX::XMStoreFloat4x4(&objConstants.ViewProj, viewProj);
	for (size_t i = 0; i < shadowConstants.size(); i++)
		DirectX::XMStoreFloat4x4(&objConstants.ShadowTransform[i], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&shadowConstants[i].ShadowTransform)));
	XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
	DirectX::XMStoreFloat3(&objConstants.EyePosW, mApp->GetMainCamera()->GetPosition());

	mMaterial->CopyData(0, objConstants);

	for (size_t i = 0; i < shadowConstants.size(); i++)
	{
		XMStoreFloat4x4(&objConstants.ViewProj, shadowConstants[i].ViewProj);
		objConstants.EyePosW = shadowConstants[i].EyePosW;

		mMaterial->CopyData(i + 1, objConstants);
	}
}

void StaticObject::Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList, int cbOffset)
{
	mMaterial->SetRenderState(cbOffset);

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

void StaticObject::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

}
