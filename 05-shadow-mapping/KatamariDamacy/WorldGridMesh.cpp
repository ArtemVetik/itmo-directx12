#include "WorldGridMesh.h"

WorldGridMesh::WorldGridMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	mDevice = device;
	mCommandList = commandList;
}

void WorldGridMesh::Build()
{
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	int index = 0;

	for (int i = 0; i <= 20; i++)
	{
		vertices.push_back({ DirectX::XMFLOAT3(-10.0f, 0.0f , i / 2.0) });
		vertices.push_back({ DirectX::XMFLOAT3(10.0f, 0.0f , i / 2.0) });

		vertices.push_back({ DirectX::XMFLOAT3(-10.0f, 0.0f , -i / 2.0) });
		vertices.push_back({ DirectX::XMFLOAT3(10.0f, 0.0f , -i / 2.0) });

		vertices.push_back({ DirectX::XMFLOAT3(i / 2.0, 0.0f , 10.0f) });
		vertices.push_back({ DirectX::XMFLOAT3(i / 2.0, 0.0f , -10.0f) });

		vertices.push_back({ DirectX::XMFLOAT3(-i / 2.0, 0.0f , 10.0f) });
		vertices.push_back({ DirectX::XMFLOAT3(-i / 2.0, 0.0f , -10.0f) });

		for (size_t j = 0; j < 8; j++)
			indices.push_back(index++);
	}
	vertices.push_back({ DirectX::XMFLOAT3(0.0f, 0.01f , 0.0f) });
	vertices.push_back({ DirectX::XMFLOAT3(1.0f, 0.01f , 0.0f) });
	vertices.push_back({ DirectX::XMFLOAT3(0.0f, 0.01f , 0.0f) });
	vertices.push_back({ DirectX::XMFLOAT3(0.0f, 1.01f , 0.0f) });
	vertices.push_back({ DirectX::XMFLOAT3(0.0f, 0.01f , 0.0f) });
	vertices.push_back({ DirectX::XMFLOAT3(0.0f, 0.01f , 1.0f) });

	for (size_t j = 0; j < 6; j++)
		indices.push_back(index++);

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mGeometry = std::make_unique<MeshGeometry>();
	mGeometry->Name = "circle";

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

	mGeometry->DrawArgs["circle01"] = submesh;
}

D3D12_VERTEX_BUFFER_VIEW WorldGridMesh::GetVertexBufferView() const
{
	return mGeometry->VertexBufferView();
}

D3D12_INDEX_BUFFER_VIEW WorldGridMesh::GetIndexBufferView() const
{
	return mGeometry->IndexBufferView();
}

std::unordered_map<std::string, SubmeshGeometry> WorldGridMesh::GetDrawArgs() const
{
	return mGeometry->DrawArgs;
}

UINT WorldGridMesh::GetVertexCount() const
{
	return 800;
}

DirectX::BoundingBox WorldGridMesh::GetBoundingBox() const
{
	return DirectX::BoundingBox();
}
