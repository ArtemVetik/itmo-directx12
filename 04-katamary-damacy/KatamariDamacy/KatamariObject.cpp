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

void KatamariObject::Update(const GameTimer& t, DirectX::XMMATRIX viewProj)
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
	DirectX::XMStoreFloat4x4(&objConstants.ViewProj, viewProj);
	XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
	DirectX::XMStoreFloat3(&objConstants.EyePosW, mApp->GetMainCamera()->GetPosition());
	objConstants.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	objConstants.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	objConstants.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	objConstants.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	objConstants.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	objConstants.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	objConstants.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };
	objConstants.Lights[3].Position = { 0.0f, 1.0f, 0.0f };
	objConstants.Lights[3].Strength = { 2.15f, 2.15f, 2.15f };
	objConstants.Lights[3].FalloffStart = 0.0f;
	objConstants.Lights[3].FalloffStart = 10.0f;

	mMaterial->CopyData(0, objConstants);

	mStartBoundingBox.Transform(mCollision, transform);
}

void KatamariObject::Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList)
{
	mMaterial->SetRenderState();

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
