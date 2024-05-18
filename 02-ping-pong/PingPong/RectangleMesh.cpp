#include "RectangleMesh.h"

RectangleMesh::RectangleMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	mDevice = device;
	mCommandList = commandList;
}

void RectangleMesh::Build()
{
	std::array<Vertex, 4> vertices =
	{
		Vertex({ DirectX::XMFLOAT3(-0.05f, -0.1f , 0.0f), DirectX::XMFLOAT4(DirectX::Colors::Orange) }),
		Vertex({ DirectX::XMFLOAT3(-0.05f, +0.1f , 0.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
		Vertex({ DirectX::XMFLOAT3(+0.05f, +0.1f , 0.0f), DirectX::XMFLOAT4(DirectX::Colors::Orange) }),
		Vertex({ DirectX::XMFLOAT3(+0.05f, -0.1f , 0.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) })
	};

	std::array<std::uint16_t, 6> indices =
	{
		0, 1, 2,
		2, 3, 0,
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mGeometry = std::make_unique<MeshGeometry>();
	mGeometry->Name = "rectangle";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mGeometry->VertexBufferCPU));
	CopyMemory(mGeometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mGeometry->IndexBufferCPU));
	CopyMemory(mGeometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mGeometry->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(mDevice,
		mCommandList, vertices.data(), vbByteSize, mGeometry->VertexBufferUploader);

	mGeometry->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(mDevice,
		mCommandList, indices.data(), ibByteSize, mGeometry->IndexBufferUploader);

	mGeometry->VertexByteStride = sizeof(Vertex);
	mGeometry->VertexBufferByteSize = vbByteSize;
	mGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	mGeometry->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mGeometry->DrawArgs["rectangle01"] = submesh;
}

D3D12_VERTEX_BUFFER_VIEW RectangleMesh::GetVertexBufferView() const
{
	return mGeometry->VertexBufferView();
}

D3D12_INDEX_BUFFER_VIEW RectangleMesh::GetIndexBufferView() const
{
	return mGeometry->IndexBufferView();
}

std::unordered_map<std::string, SubmeshGeometry> RectangleMesh::GetDrawArgs() const
{
	return mGeometry->DrawArgs;
}

UINT RectangleMesh::GetVertexCount() const
{
	return 4;
}
