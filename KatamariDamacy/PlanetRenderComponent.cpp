#include "PlanetRenderComponent.h"

PlanetRenderComponent::PlanetRenderComponent(Mesh* mesh, Shader* shader, PlanetSettings settings)
{
	mMesh = mesh;
	mShader = shader;
	mSettings = settings;
	mAroundAngle = settings.StartAngle;
	mSelfAngle = 0.0f;
	mPosition = {};
	mRotation = DirectX::XMQuaternionIdentity();
}

void PlanetRenderComponent::Build()
{

}

void PlanetRenderComponent::Update(const GameTimer& t, DirectX::XMMATRIX viewProj)
{
	mAroundAngle += t.DeltaTime() * mSettings.RotateAroundSpeed;
	mSelfAngle += t.DeltaTime() * mSettings.RotateSelfSpeed;

	mPosition = DirectX::XMVectorAdd(
		mSettings.Root->GetCurrentPositionVector(),
		{
			sinf(DirectX::XM_PIDIV2 + sinf(mAroundAngle) * mSettings.OrbitAngle) * cosf(mAroundAngle) * mSettings.RotateRadius,
			cosf(DirectX::XM_PIDIV2 + sinf(mAroundAngle) * mSettings.OrbitAngle) * mSettings.RotateRadius,
			sinf(DirectX::XM_PIDIV2 + sinf(mAroundAngle) * mSettings.OrbitAngle) * sinf(mAroundAngle) * mSettings.RotateRadius,
		}
	);

	DirectX::XMVECTOR upVector = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), mSettings.Rotation);
	DirectX::XMVECTOR upRotation = DirectX::XMQuaternionRotationAxis(upVector, mSelfAngle);

	mRotation = DirectX::XMQuaternionMultiply(mSettings.Rotation, upRotation);

	DirectX::XMMATRIX transform = DirectX::XMMatrixAffineTransformation(
		mSettings.Scale,
		{},
		mRotation,
		mPosition
	);

	DirectX::XMMATRIX texTransform = XMLoadFloat4x4(&MathHelper::Identity4x4());

	ObjectConstants objConstants;
	DirectX::XMStoreFloat4x4(&objConstants.WorldViewProj, DirectX::XMMatrixMultiply(viewProj, DirectX::XMMatrixTranspose(transform)));
	XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

	mShader->CbCopyData(0, objConstants);
}

void PlanetRenderComponent::Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList)
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

void PlanetRenderComponent::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
}

DirectX::XMFLOAT3 PlanetRenderComponent::GetCurrentPosition()
{
	DirectX::XMFLOAT3 position;
	DirectX::XMStoreFloat3(&position, mPosition);
	return position;
}

DirectX::XMVECTOR PlanetRenderComponent::GetCurrentPositionVector()
{
	return mPosition;
}

DirectX::XMVECTOR PlanetRenderComponent::GetCurrentQuaternionRotation()
{
	return mRotation;
}
