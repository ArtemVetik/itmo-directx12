#pragma once
#include "Mesh.h"
#include "../Common/GeometryGenerator.h"

class SSQuadMesh : public Mesh
{
public:
	SSQuadMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, GeometryGenerator::MeshData meshData);
	void Build() override;
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const override;
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const override;
	std::unordered_map<std::string, SubmeshGeometry> GetDrawArgs() const override;
	UINT GetVertexCount() const override;
	DirectX::BoundingBox GetBoundingBox() const override;

private:
	GeometryGenerator::MeshData mMeshData;
	std::unique_ptr<MeshGeometry> mGeometry = nullptr;
	ID3D12Device* mDevice;
	ID3D12GraphicsCommandList* mCommandList;

};

