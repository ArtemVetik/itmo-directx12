#pragma once

#include "../Common/d3dUtil.h"

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
	DirectX::XMFLOAT2 TexC;
};

class Mesh
{
public:
	virtual void Build() = 0;
	virtual D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const = 0;
	virtual D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const = 0;
	virtual std::unordered_map<std::string, SubmeshGeometry> GetDrawArgs() const = 0;
	virtual UINT GetVertexCount() const = 0;
	virtual DirectX::BoundingBox GetBoundingBox() const = 0;
};