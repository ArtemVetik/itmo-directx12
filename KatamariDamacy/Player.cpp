#include "Player.h"

Player::Player(Camera* camera, DefaultMaterial* material, Mesh* mesh, std::vector<KatamariObject*> objects)
{
	mCamera = camera;
	mMaterial = material;
	mMesh = mesh;
	mPosition = { 0, 0, 0 };
	mLook = { 0.0f, 0.0f, 1.0f };
	mRotation = DirectX::XMQuaternionIdentity();
	mFreeObjects = objects;
	
	auto meshBox = mesh->GetBoundingBox();
	mCollision.Center = meshBox.Center;
	mCollision.Extents = meshBox.Extents;
	DirectX::XMStoreFloat4(&mCollision.Orientation, DirectX::XMQuaternionIdentity());
}

void Player::Build()
{
}

void Player::Update(const GameTimer& t, DirectX::XMMATRIX viewProj)
{
	const float dt = t.DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
	{
		DirectX::XMVECTOR s = DirectX::XMVectorReplicate(10 * dt);
		mPosition = DirectX::XMVectorMultiplyAdd(s, mLook, mPosition);

		DirectX::XMVECTOR right = DirectX::XMVector3Cross({ 0.0f, 1.0f, 0.0f }, mLook);
		right = DirectX::XMVector3Normalize(right);
		mRotation = DirectX::XMQuaternionMultiply(mRotation, DirectX::XMQuaternionRotationAxis(right, 5.0f * dt));
	}

	if (GetAsyncKeyState('S') & 0x8000)
	{
		DirectX::XMVECTOR s = DirectX::XMVectorReplicate(-10 * dt);
		mPosition = DirectX::XMVectorMultiplyAdd(s, mLook, mPosition);

		DirectX::XMVECTOR right = DirectX::XMVector3Cross(mLook, { 0.0f, 1.0f, 0.0f });
		right = DirectX::XMVector3Normalize(right);
		mRotation = DirectX::XMQuaternionMultiply(mRotation, DirectX::XMQuaternionRotationAxis(right, 5.0f * dt));
	}

	if (GetAsyncKeyState('A') & 0x8000)
	{
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(-3.0f * dt);
		mLook = XMVector3TransformNormal(mLook, R);
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(3.0f * dt);
		mLook = XMVector3TransformNormal(mLook, R);
	}

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 target;

	DirectX::XMVECTOR horizontal = DirectX::XMVectorMultiplyAdd(mLook, { -12.0f, -12.0f, -12.0f }, mPosition);
	DirectX::XMVECTOR total = DirectX::XMVectorAdd(horizontal, { 0, 5, 0 });

	DirectX::XMStoreFloat3(&position, total);
	DirectX::XMStoreFloat3(&target, DirectX::XMVectorAdd(DirectX::XMVectorAdd(mPosition, mLook), { 0, 0, 0 }));

	mCamera->SetPosition(position);
	mCamera->LookAt(position, target, { 0, 1, 0 });

	mCamera->UpdateViewMatrix();


	DirectX::XMMATRIX transform = DirectX::XMMatrixAffineTransformation(
		{ 0.05f, 0.05f, 0.05f },
		{ },
		mRotation,
		mPosition
	);

	DirectX::XMMATRIX texTransform = XMLoadFloat4x4(&MathHelper::Identity4x4());

	ObjectConstants objConstants;
	DirectX::XMStoreFloat4x4(&objConstants.WorldViewProj, DirectX::XMMatrixMultiply(viewProj, DirectX::XMMatrixTranspose(transform)));
	XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

	mMaterial->CopyData(0, objConstants);

	DirectX::BoundingOrientedBox actualCollision = mCollision;
	mCollision.Transform(actualCollision, transform);

	for (size_t j = 0; j < mConnectedObjects.size(); j++)
	{
		for (int i = mFreeObjects.size() - 1; i >= 0; i--)
		{
			auto o = mConnectedObjects[j];
			if (o->GetBoundingBox().Intersects(mFreeObjects[i]->GetBoundingBox()))
			{
				mFreeObjects[i]->SetParent(o);
				mConnectedObjects.push_back(mFreeObjects[i]);
				mFreeObjects.erase(mFreeObjects.begin() + i);
			}
		}
	}

	for (int i = mFreeObjects.size() - 1; i >= 0; i--)
	{
		if (actualCollision.Intersects(mFreeObjects[i]->GetBoundingBox()))
		{
			mFreeObjects[i]->SetParent(this);
			mConnectedObjects.push_back(mFreeObjects[i]);
			mFreeObjects.erase(mFreeObjects.begin() + i);
		}
	}
}

void Player::Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList)
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

void Player::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
}

DirectX::XMMATRIX Player::GetTransformMatrix()
{
	return DirectX::XMMatrixAffineTransformation(
		{ 1, 1, 1 },
		{ },
		mRotation,
		mPosition
	);
}

DirectX::XMVECTOR Player::GetPosition()
{
	return mPosition;
}

DirectX::XMVECTOR Player::GetRotation()
{
	return mRotation;
}

Transformable* Player::GetParent()
{
	return nullptr;
}
