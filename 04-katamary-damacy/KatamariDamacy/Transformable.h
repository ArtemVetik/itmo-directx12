#pragma once

#include <DirectXMath.h>

class Transformable
{
public:
	virtual DirectX::XMVECTOR GetPosition() = 0;
	virtual DirectX::XMVECTOR GetRotation() = 0;
	virtual DirectX::XMMATRIX GetTransformMatrix() = 0;
	virtual Transformable* GetParent() = 0;
};