#include "CircleMesh.h"

CircleMesh::CircleMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	mDevice = device;
	mCommandList = commandList;
}

void CircleMesh::Build()
{
	static const unsigned int sides = 32;
	float constexpr angle = DirectX::XMConvertToRadians(360.0f / sides);

	float Cos = cos(angle);
	float Sin = sin(angle);

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	vertices.push_back({ DirectX::XMFLOAT3(0.0f, 0.0f , 0.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) });

	for (int i = sides; i >= 0; --i)
	{
		if (i >= 1) 
		{
			indices.push_back(0);
			indices.push_back(sides - i + 1);
			indices.push_back(sides - i + 2);
		}

		float x = cos(i * angle) * 0.05f;
		float y = sin(i * angle) * 0.05f;
		vertices.push_back({ DirectX::XMFLOAT3(x, y , 0.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan) });
	}

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

D3D12_VERTEX_BUFFER_VIEW CircleMesh::GetVertexBufferView() const
{
	return mGeometry->VertexBufferView();
}

D3D12_INDEX_BUFFER_VIEW CircleMesh::GetIndexBufferView() const
{
	return mGeometry->IndexBufferView();
}

std::unordered_map<std::string, SubmeshGeometry> CircleMesh::GetDrawArgs() const
{
	return mGeometry->DrawArgs;
}

UINT CircleMesh::GetVertexCount() const
{
	return 33;
}
