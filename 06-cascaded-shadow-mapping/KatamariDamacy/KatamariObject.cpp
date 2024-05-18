#include "KatamariObject.h"

KatamariObject::KatamariObject(Mesh* mesh, DefaultMaterial* material, KatamariObjectSettings settings, KatamariApp* app)
{
	mMesh = mesh;
	mMaterial = material;
	mSettings = settings;
	mPosition = settings.Position;
	mRotation = settings.Rotation;
	mApp = app;
	mParent = nullptr;

	auto meshBox = mesh->GetBoundingBox();
	mStartBoundingBox.Center = meshBox.Center;
	mStartBoundingBox.Extents = meshBox.Extents;
	DirectX::XMStoreFloat4(&mStartBoundingBox.Orientation, DirectX::XMQuaternionIdentity());

	mCollision = mStartBoundingBox;
}

void KatamariObject::Build()
{

}

void KatamariObject::Update(const GameTimer& t, DirectX::XMMATRIX viewProj, std::vector<ShadowMapConstants> shadowConstants)
{
	DirectX::XMMATRIX transform = DirectX::XMMatrixAffineTransformation(
		mSettings.Scale,
		{},
		mRotation,
		mPosition
	);

	if (mParent != nullptr)
		transform = DirectX::XMMatrixMultiply(transform, mParent->GetTransformMatrix());

	DirectX::XMMATRIX texTransform = XMLoadFloat4x4(&MathHelper::Identity4x4());

	ObjectConstants objConstants{};
	XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(transform));
	XMStoreFloat4x4(&objConstants.ViewProj, viewProj);

	for (size_t i = 0; i < shadowConstants.size(); i++)
		DirectX::XMStoreFloat4x4(&objConstants.ShadowTransform[i], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&shadowConstants[i].ShadowTransform)));

	XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
	XMStoreFloat3(&objConstants.EyePosW, mApp->GetMainCamera()->GetPosition());

	mMaterial->CopyData(0, objConstants);

	for (size_t i = 0; i < shadowConstants.size(); i++)
	{
		XMStoreFloat4x4(&objConstants.ViewProj, shadowConstants[i].ViewProj);
		objConstants.EyePosW = shadowConstants[i].EyePosW;

		mMaterial->CopyData(i + 1, objConstants);
	}

	mStartBoundingBox.Transform(mCollision, transform);
}

void KatamariObject::Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList, int cbOffset)
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

		commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	}
}

void KatamariObject::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
}

DirectX::BoundingOrientedBox KatamariObject::GetBoundingBox() const
{
	return mCollision;
}

void KatamariObject::SetParent(Transformable* parent)
{
	mParent = parent;

	mPosition = DirectX::XMVector3TransformCoord(mPosition, DirectX::XMMatrixInverse(nullptr, mParent->GetTransformMatrix()));

	DirectX::XMVECTOR localConjugate = DirectX::XMQuaternionConjugate(mParent->GetRotation());
	mRotation = DirectX::XMQuaternionMultiply(mRotation, localConjugate);
}

DirectX::XMMATRIX KatamariObject::GetTransformMatrix()
{
	DirectX::XMMATRIX matrix = DirectX::XMMatrixAffineTransformation(
		{ 1, 1, 1 },
		{},
		mRotation,
		mPosition
	);

	if (mParent != nullptr)
		matrix = DirectX::XMMatrixMultiply(matrix, mParent->GetTransformMatrix());

	return matrix;
}

DirectX::XMVECTOR KatamariObject::GetPosition()
{
	return mPosition;
}

DirectX::XMVECTOR KatamariObject::GetRotation()
{
	DirectX::XMVECTOR rotation = mRotation;

	if (mParent)
		rotation = DirectX::XMQuaternionMultiply(rotation, mParent->GetRotation());

	return rotation;
}

Transformable* KatamariObject::GetParent()
{
	return mParent;
}
