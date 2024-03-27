#include "SSQuadMesh.h"
#include "../Common/GeometryGenerator.h"

SSQuadMesh::SSQuadMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	mDevice = device;
	mCommandList = commandList;
}

void SSQuadMesh::Build()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData quad = geoGen.CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	for (size_t i = 0; i < quad.Vertices.size(); i++)
		vertices.push_back({ quad.Vertices[i].Position, quad.Vertices[i].Normal, quad.Vertices[i].TexC});

	for (size_t i = 0; i < quad.Indices32.size(); i++)
		indices.push_back(quad.Indices32[i]);

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mGeometry = std::make_unique<MeshGeometry>();
	mGeometry->Name = "quad";

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

	mGeometry->DrawArgs["quad01"] = submesh;
}

D3D12_VERTEX_BUFFER_VIEW SSQuadMesh::GetVertexBufferView() const
{
	return mGeometry->VertexBufferView();
}

D3D12_INDEX_BUFFER_VIEW SSQuadMesh::GetIndexBufferView() const
{
	return mGeometry->IndexBufferView();
}

std::unordered_map<std::string, SubmeshGeometry> SSQuadMesh::GetDrawArgs() const
{
	return mGeometry->DrawArgs;
}

UINT SSQuadMesh::GetVertexCount() const
{
    return 8;
}

DirectX::BoundingBox SSQuadMesh::GetBoundingBox() const
{
    return DirectX::BoundingBox();
}
