#pragma once

#include <d3d12.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Mesh.h"

class FileMesh : public Mesh
{
public:
	FileMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const char* path);
	void Build() override;
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const override;
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const override;
	std::unordered_map<std::string, SubmeshGeometry> GetDrawArgs() const override;
	UINT GetVertexCount() const override;
	DirectX::BoundingBox GetBoundingBox() const override;

private:
	std::unique_ptr<MeshGeometry> mGeometry = nullptr;
	DirectX::BoundingBox mBoundingBox;
	
	ID3D12Device* mDevice;
	ID3D12GraphicsCommandList* mCommandList;
	const char* mPath;
};

