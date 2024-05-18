#pragma once
#include "Mesh.h"

class WorldGridMesh : public Mesh
{
public:
	WorldGridMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void Build() override;
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const override;
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const override;
	std::unordered_map<std::string, SubmeshGeometry> GetDrawArgs() const override;
	UINT GetVertexCount() const override;

private:
	std::unique_ptr<MeshGeometry> mGeometry = nullptr;
	ID3D12Device* mDevice;
	ID3D12GraphicsCommandList* mCommandList;
};

