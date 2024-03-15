#pragma once

#include <DirectXMath.h>

class PlanetRoot
{
public:
	virtual DirectX::XMFLOAT3 GetCurrentPosition() = 0;
	virtual DirectX::XMVECTOR GetCurrentPositionVector() = 0;
	virtual DirectX::XMVECTOR GetCurrentQuaternionRotation() = 0;
};

class PointPlanetRoot : public PlanetRoot
{
public:
	PointPlanetRoot(DirectX::XMFLOAT3 poisition) { mPosition = poisition; }
	virtual DirectX::XMFLOAT3 GetCurrentPosition() override { return mPosition; }
	virtual DirectX::XMVECTOR GetCurrentPositionVector() override { return DirectX::XMLoadFloat3(&mPosition); }
	DirectX::XMVECTOR GetCurrentQuaternionRotation() override { return DirectX::XMQuaternionIdentity(); }
	
private:
	DirectX::XMFLOAT3 mPosition;
};