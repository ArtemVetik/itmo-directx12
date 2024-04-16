#include "FileMesh.h"

FileMesh::FileMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const char* path)
{
	mDevice = device;
	mCommandList = commandList;
	mPath = path;
}

#include <iostream>

void FileMesh::Build()
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(mPath, aiProcessPreset_TargetRealtime_Fast | aiProcess_GenBoundingBoxes);

	auto aabb = scene->mMeshes[0]->mAABB;

	DirectX::XMVECTOR minPoint = DirectX::XMVectorSet(aabb.mMin.x, aabb.mMin.y, aabb.mMin.z, 1.0f);
	DirectX::XMVECTOR maxPoint = DirectX::XMVectorSet(aabb.mMax.x, aabb.mMax.y, aabb.mMax.z, 1.0f);

	DirectX::XMVECTOR extents = DirectX::XMVectorScale(DirectX::XMVectorSubtract(maxPoint, minPoint), 0.5f);
	DirectX::XMStoreFloat3(&mBoundingBox.Extents, extents);

	DirectX::XMVECTOR center = DirectX::XMVectorAdd(minPoint, DirectX::XMLoadFloat3(&mBoundingBox.Extents));
	DirectX::XMStoreFloat3(&mBoundingBox.Center, center);

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	int vc = -1;
	for (int j(0); j < AI_MAX_NUMBER_OF_COLOR_SETS; j++)
	{
		if (scene->mMeshes[0]->HasVertexColors(j)) {
			vc = j;
			break;
		}
	}

	for (size_t i = 0; i < scene->mMeshes[0]->mNumVertices; i++)
	{
		auto vertex = scene->mMeshes[0]->mVertices[i];
		Vertex v;
		v.Pos.x = vertex.x;
		v.Pos.y = vertex.y;
		v.Pos.z = vertex.z;

		auto normal = scene->mMeshes[0]->mNormals[i];
		v.Normal = { normal.x, normal.y, normal.z };

		auto tex = scene->mMeshes[0]->mTextureCoords[0][i];

		v.TexC.x = tex.x;
		v.TexC.y = 1.0f - tex.y;

		vertices.push_back(v);
	}

	for (size_t i = 0; i < scene->mMeshes[0]->mNumFaces; i++)
	{
		for (size_t k = 0; k < scene->mMeshes[0]->mFaces[i].mNumIndices; k++)
		{
			indices.push_back(scene->mMeshes[0]->mFaces[i].mIndices[k]);
		}
	}

	vertices.push_back({ {	mBoundingBox.Center.x - mBoundingBox.Extents.x,
							mBoundingBox.Center.y - mBoundingBox.Extents.y,
							mBoundingBox.Center.z - mBoundingBox.Extents.z} , { } , { } });

	vertices.push_back({ {	mBoundingBox.Center.x - mBoundingBox.Extents.x,
							mBoundingBox.Center.y - mBoundingBox.Extents.y,
							mBoundingBox.Center.z + mBoundingBox.Extents.z} , { } , { } });

	vertices.push_back({ {	mBoundingBox.Center.x + mBoundingBox.Extents.x,
							mBoundingBox.Center.y - mBoundingBox.Extents.y,
							mBoundingBox.Center.z - mBoundingBox.Extents.z} , { } , { } });

	vertices.push_back({ {	mBoundingBox.Center.x + mBoundingBox.Extents.x,
							mBoundingBox.Center.y - mBoundingBox.Extents.y,
							mBoundingBox.Center.z + mBoundingBox.Extents.z} , { } , { } });

	vertices.push_back({ {	mBoundingBox.Center.x - mBoundingBox.Extents.x,
							mBoundingBox.Center.y + mBoundingBox.Extents.y,
							mBoundingBox.Center.z - mBoundingBox.Extents.z} , { } , { } });

	vertices.push_back({ {	mBoundingBox.Center.x - mBoundingBox.Extents.x,
							mBoundingBox.Center.y + mBoundingBox.Extents.y,
							mBoundingBox.Center.z + mBoundingBox.Extents.z} , { } , { } });

	vertices.push_back({ {	mBoundingBox.Center.x + mBoundingBox.Extents.x,
							mBoundingBox.Center.y + mBoundingBox.Extents.y,
							mBoundingBox.Center.z - mBoundingBox.Extents.z} , { } , { } });

	vertices.push_back({ {	mBoundingBox.Center.x + mBoundingBox.Extents.x,
							mBoundingBox.Center.y + mBoundingBox.Extents.y,
							mBoundingBox.Center.z + mBoundingBox.Extents.z} , { } , { } });

	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(3);
	indices.push_back(2);
	indices.push_back(0);
	indices.push_back(4);
	indices.push_back(5);
	indices.push_back(7);
	indices.push_back(6);
	indices.push_back(4);
	indices.push_back(1);
	indices.push_back(5);
	indices.push_back(3);
	indices.push_back(7);
	indices.push_back(2);
	indices.push_back(6);

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mGeometry = std::make_unique<MeshGeometry>();
	mGeometry->Name = "ffsphere";

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
	submesh.IndexCount = (UINT)indices.size() - 16;
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mGeometry->DrawArgs["ffsphere01"] = submesh;

	SubmeshGeometry submesh2;
	submesh2.IndexCount = 16;
	submesh2.StartIndexLocation = (UINT)indices.size() - 16;
	submesh2.BaseVertexLocation = vertices.size() - 8;

	mGeometry->DrawArgs["boundingBox"] = submesh2;
}

D3D12_VERTEX_BUFFER_VIEW FileMesh::GetVertexBufferView() const
{
	return mGeometry->VertexBufferView();
	/*auto a = mModel.get();
	auto part = mModel->meshes[0]->opaqueMeshParts[0]->vertexBuffer;

	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = part.GpuAddress();
	vbv.StrideInBytes = mModel->meshes[0]->opaqueMeshParts[0]->vertexStride;
	vbv.SizeInBytes = mModel->meshes[0]->opaqueMeshParts[0]->vertexBufferSize;

	return vbv;*/
}

D3D12_INDEX_BUFFER_VIEW FileMesh::GetIndexBufferView() const
{
	return mGeometry->IndexBufferView();
	/*auto part = mModel->meshes[0]->opaqueMeshParts[0]->indexBuffer;

	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = part.GpuAddress();
	ibv.Format = mModel->meshes[0]->opaqueMeshParts[0]->indexFormat;
	ibv.SizeInBytes = mModel->meshes[0]->opaqueMeshParts[0]->indexBufferSize;

	return ibv;*/
}

std::unordered_map<std::string, SubmeshGeometry> FileMesh::GetDrawArgs() const
{
	return mGeometry->DrawArgs;
	/*std::unordered_map<std::string, SubmeshGeometry> args;
	SubmeshGeometry mesh;
	mesh.IndexCount = mModel->meshes[0]->opaqueMeshParts[0]->indexCount;

	args.insert(std::make_pair("filePart", mesh));

	return args;*/
}

UINT FileMesh::GetVertexCount() const
{
	return 1110;
	//return mModel->meshes[0]->opaqueMeshParts[0]->vertexCount;
}

DirectX::BoundingBox FileMesh::GetBoundingBox() const
{
	return mBoundingBox;
}
