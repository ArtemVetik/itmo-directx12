#include "Game.h"

const int gNumberFrameResources = 3;

Game::Game(HINSTANCE hInstance) : DXCore(hInstance)
{
	mainCamera = new Camera(screenWidth, screenHeight);
}

Game::~Game()
{
	if (Device != nullptr)
		FlushCommandQueue();

	delete systemData;
	systemData = 0;

	delete inputManager;

	delete emitter;
}

bool Game::Initialize()
{
	if (!DXCore::Initialize())
		return false;

	// reset the command list to prep for initialization commands
	ThrowIfFailed(CommandList->Reset(CommandListAllocator.Get(), nullptr));

	systemData = new SystemData();

	inputManager = new InputManager();

	emitter = new Emitter(
		1000000,
		100,
		1000000.0f, // Particles per second
		1000.0f, // Particle lifetime
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f),
		XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)
	);

	BuildShapeGeometry();
	BuildUAVs();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildFrameResources();
	BuildPSOs();
	BuildRenderItems();

	// execute the initialization commands
	ThrowIfFailed(CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	ThrowIfFailed(CommandListAllocator->Reset());

	ThrowIfFailed(CommandList->Reset(CommandListAllocator.Get(), PSOs["particleDeadList"].Get()));

	CommandList->SetComputeRootSignature(particleRootSignature.Get());

	currentFrameResourceIndex = (currentFrameResourceIndex + 1) % gNumberFrameResources;
	currentFrameResource = FrameResources[currentFrameResourceIndex].get();

	UpdateMainPassCB(timer);

	ID3D12DescriptorHeap* descriptorHeaps[] = { SRVUAVHeap.Get() };
	CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	auto objectCB = currentFrameResource->ObjectCB->Resource();
	CommandList->SetComputeRootConstantBufferView(0, objectCB->GetGPUVirtualAddress());

	auto timeCB = currentFrameResource->TimeCB->Resource();
	CommandList->SetComputeRootConstantBufferView(1, timeCB->GetGPUVirtualAddress());

	auto particleCB = currentFrameResource->ParticleCB->Resource();
	CommandList->SetComputeRootConstantBufferView(2, particleCB->GetGPUVirtualAddress());

	CommandList->SetComputeRootDescriptorTable(3, ParticlePoolGPUUAV);
	CommandList->SetComputeRootDescriptorTable(4, ACDeadListGPUUAV);
	CommandList->SetComputeRootDescriptorTable(5, DrawListGPUUAV);
	CommandList->SetComputeRootDescriptorTable(6, DrawArgsGPUUAV);

	CommandList->Dispatch(emitter->GetMaxParticles(), 1, 1);

	ThrowIfFailed(CommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists1[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists1);

	// Wait for the work to finish.
	FlushCommandQueue();
}

void Game::Resize()
{
	DXCore::Resize();

	mainCamera->SetProjectionMatrix(screenWidth, screenHeight);
}

void Game::Update(const Timer& timer)
{
	mainCamera->Update();
	inputManager->UpdateController();

	// Cycle through the circular frame resource array.
	currentFrameResourceIndex = (currentFrameResourceIndex + 1) % gNumberFrameResources;
	currentFrameResource = FrameResources[currentFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (currentFrameResource->Fence != 0 && Fence->GetCompletedValue() < currentFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(Fence->SetEventOnCompletion(currentFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	emitter->Update(timer.GetTotalTime(), timer.GetTotalTime());
	UpdateMainPassCB(timer);
}

void Game::Draw(const Timer& timer)
{
	auto currentCommandListAllocator = currentFrameResource->commandListAllocator;

	// reuse the memory associated with command recording
	// we can only reset when the associated command lists have finished execution on the GPU
	ThrowIfFailed(currentCommandListAllocator->Reset());

	ThrowIfFailed(CommandList->Reset(currentCommandListAllocator.Get(), PSOs["opaque"].Get()));

	CommandList->SetPipelineState(PSOs["particleEmit"].Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { SRVUAVHeap.Get() };
	CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	CommandList->SetComputeRootSignature(particleRootSignature.Get());

	auto objectCB = currentFrameResource->ObjectCB->Resource();
	CommandList->SetComputeRootConstantBufferView(0, objectCB->GetGPUVirtualAddress());

	auto timeCB = currentFrameResource->TimeCB->Resource();
	CommandList->SetComputeRootConstantBufferView(1, timeCB->GetGPUVirtualAddress());

	auto particleCB = currentFrameResource->ParticleCB->Resource();
	CommandList->SetComputeRootConstantBufferView(2, particleCB->GetGPUVirtualAddress());

	CommandList->SetComputeRootDescriptorTable(3, ParticlePoolGPUUAV);
	CommandList->SetComputeRootDescriptorTable(4, ACDeadListGPUUAV);
	CommandList->SetComputeRootDescriptorTable(5, DrawListGPUUAV);
	CommandList->SetComputeRootDescriptorTable(6, DrawArgsGPUUAV);

	while (emitter->GetEmitTimeCounter() >= emitter->GetTimeBetweenEmit())
	{
		emitter->SetEmitCount((int)(emitter->GetEmitTimeCounter() / emitter->GetTimeBetweenEmit()));

		emitter->SetEmitCount(min(emitter->GetEmitCount(), 65535));
		emitter->SetEmitTimeCounter(fmod(emitter->GetEmitTimeCounter(), emitter->GetTimeBetweenEmit()));

		UpdateMainPassCB(timer);

		CommandList->Dispatch(emitter->GetEmitCount(), 1, 1);
	}

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RWDrawList.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));

	CommandList->CopyResource(RWDrawList.Get(), DrawListUploadBuffer.Get());

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RWDrawList.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	CommandList->SetPipelineState(PSOs["particleUpdate"].Get());
	CommandList->SetComputeRootSignature(particleRootSignature.Get());
	CommandList->Dispatch(emitter->GetMaxParticles(), 1, 1);

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(RWDrawList.Get()));

	CommandList->SetPipelineState(PSOs["particleDraw"].Get());
	CommandList->SetComputeRootSignature(particleRootSignature.Get());
	CommandList->Dispatch(1, 1, 1);

	CommandList->RSSetViewports(1, &ScreenViewPort);
	CommandList->RSSetScissorRects(1, &ScissorRect);

	// indicate a state transition on the resource usage
	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(DepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// clear the back buffer and depth buffer
	CommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Black, 0, nullptr);
	CommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDesc(RTVHeap->GetCPUDescriptorHandleForHeapStart(), SwapChainBufferCount, RTVDescriptorSize);

	for (int i = 0; i < GBufferCount; i++) {
		CommandList->ClearRenderTargetView(rtvDesc, DirectX::Colors::Black, 0, nullptr);
		rtvDesc.Offset(1, RTVDescriptorSize);
	}

	rtvDesc = CD3DX12_CPU_DESCRIPTOR_HANDLE(RTVHeap->GetCPUDescriptorHandleForHeapStart(), SwapChainBufferCount, RTVDescriptorSize);
	CommandList->OMSetRenderTargets(GBufferCount, &rtvDesc, true, &DepthStencilView());

	for (int i = 0; i < GBufferCount; i++)
	{
		CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GBuffer[i].Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}
	
	CommandList->SetPipelineState(PSOs["geoOpaque"].Get());

	CommandList->SetGraphicsRootSignature(geoRootSignature.Get());
	auto geoObjectCB = currentFrameResource->GeoObjectCB->Resource();

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	for (size_t i = 0; i < OpaqueRitems.size(); ++i)
	{
		auto ri = OpaqueRitems[i];

		CommandList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		CommandList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		CommandList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = geoObjectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
		CommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		CommandList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}


	CommandList->SetPipelineState(PSOs["opaque"].Get());

	CommandList->SetGraphicsRootSignature(rootSignature.Get());

	CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	CommandList->SetGraphicsRootConstantBufferView(0, objectCB->GetGPUVirtualAddress());
	CommandList->SetGraphicsRootConstantBufferView(1, timeCB->GetGPUVirtualAddress());
	CommandList->SetGraphicsRootConstantBufferView(2, particleCB->GetGPUVirtualAddress());

	CommandList->SetGraphicsRootDescriptorTable(3, ParticlePoolGPUSRV);
	CommandList->SetGraphicsRootDescriptorTable(4, DrawListGPUSRV);

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RWDrawArgs.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT));

	CommandList->ExecuteIndirect(
		particleCommandSignature.Get(),
		1,
		RWDrawArgs.Get(),
		0,
		nullptr,
		0);

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RWDrawArgs.Get(),
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	rtvDesc = CD3DX12_CPU_DESCRIPTOR_HANDLE(RTVHeap->GetCPUDescriptorHandleForHeapStart(), SwapChainBufferCount + GBufferCount, RTVDescriptorSize);
	// accumulation RT
	CommandList->OMSetRenderTargets(1, &rtvDesc, true, nullptr);

	for (int i = 0; i < GBufferCount; i++)
	{
		CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GBuffer[i].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(DepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(AccumulationBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CommandList->SetPipelineState(PSOs["accumulationPass"].Get());
	CommandList->SetGraphicsRootSignature(gBufferRootSignature.Get());

	for (size_t i = 0; i < SSRitems.size(); ++i)
	{
		auto ri = SSRitems[i];

		CommandList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		CommandList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		CommandList->IASetPrimitiveTopology(ri->PrimitiveType);

		CommandList->SetGraphicsRootDescriptorTable(0, GBufferGPUSRV);

		CommandList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}

	// specify the buffers we are going to render to
	CommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, nullptr);

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(AccumulationBuffer.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

	CommandList->SetPipelineState(PSOs["renderQuad"].Get());
	CommandList->SetGraphicsRootSignature(gBufferRootSignature.Get());

	for (size_t i = 0; i < SSRitems.size(); ++i)
	{
		auto ri = SSRitems[i];

		CommandList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		CommandList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		CommandList->IASetPrimitiveTopology(ri->PrimitiveType);
		
		CommandList->SetGraphicsRootDescriptorTable(0, GBufferGPUSRV);

		CommandList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}

	// indicate a state transition on the resource usage
	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// done recording commands
	ThrowIfFailed(CommandList->Close());

	// add the command list to the queue for execution
	ID3D12CommandList* cmdsLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// wwap the back and front buffers
	ThrowIfFailed(SwapChain->Present(0, 0));

	currentBackBuffer = (currentBackBuffer + 1) % SwapChainBufferCount;

	// advance the fence value to mark commands up to this fence point
	currentFrameResource->Fence = ++currentFence;

	// add an instruction to the command queue to set a new fence point.
	// because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal()
	CommandQueue->Signal(Fence.Get(), currentFence);
}

void Game::UpdateMainPassCB(const Timer& timer)
{
	XMMATRIX world = XMMatrixIdentity();
	XMMATRIX view = XMLoadFloat4x4(&mainCamera->GetViewMatrix());
	XMMATRIX projection = XMLoadFloat4x4(&mainCamera->GetProjectionMatrix());

	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
	XMStoreFloat4x4(&objConstants.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&objConstants.Projection, XMMatrixTranspose(projection));
	objConstants.AspectRatio = (float)screenWidth / screenHeight;

	auto currentObjectCB = currentFrameResource->ObjectCB.get();
	currentObjectCB->CopyData(0, objConstants);

	// Geo Constants
	{
		auto currGeoObjectCB = currentFrameResource->GeoObjectCB.get();
		for (auto& e : AllRitems)
		{
			// Only update the cbuffer data if the constants have changed.  
			// This needs to be tracked per frame resource.
			//if (e->NumFramesDirty > 0)
			{
				XMMATRIX world = XMLoadFloat4x4(&e->World);

				ObjectConstants objConstants;
				XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
				XMStoreFloat4x4(&objConstants.View, XMMatrixTranspose(view));
				XMStoreFloat4x4(&objConstants.Projection, XMMatrixTranspose(projection));
				objConstants.AspectRatio = (float)screenWidth / screenHeight;

				currGeoObjectCB->CopyData(e->ObjCBIndex, objConstants);

				// Next FrameResource need to be updated too.
				e->NumFramesDirty--;
			}
		}
	}

	MainTimeCB.DeltaTime = timer.GetDeltaTime();
	MainTimeCB.TotalTime = timer.GetTotalTime();

	auto currentTimeCB = currentFrameResource->TimeCB.get();
	currentTimeCB->CopyData(0, MainTimeCB);

	MainParticleCB.EmitCount = emitter->GetEmitCount();
	MainParticleCB.MaxParticles = emitter->GetMaxParticles();
	MainParticleCB.GridSize = emitter->GetGridSize();
	MainParticleCB.LifeTime = emitter->GetLifeTime();
	MainParticleCB.velocity = emitter->GetVelocity();
	MainParticleCB.acceleration = emitter->GetAcceleration();

	auto currentParticleCB = currentFrameResource->ParticleCB.get();
	currentParticleCB->CopyData(0, MainParticleCB);
}

void Game::BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	//
	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.
	//

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].UV = box.Vertices[i].TexC;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].UV = grid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].UV = sphere.Vertices[i].TexC;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].UV = cylinder.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(Device.Get(),
		CommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(Device.Get(),
		CommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;

	Geometries[geo->Name] = std::move(geo);

	// Screen Space Quad
	GeometryGenerator::MeshData quad = geoGen.CreateQuad(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f);

	vertices.clear();
	indices.clear();

	for (size_t i = 0; i < quad.Vertices.size(); i++)
		vertices.push_back({ quad.Vertices[i].Position, quad.Vertices[i].Normal, quad.Vertices[i].TexC });

	for (size_t i = 0; i < quad.Indices32.size(); i++)
		indices.push_back(quad.Indices32[i]);

	auto ssQuadGeo = std::make_unique<MeshGeometry>();
	ssQuadGeo->Name = "quad";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &ssQuadGeo->VertexBufferCPU));
	CopyMemory(ssQuadGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &ssQuadGeo->IndexBufferCPU));
	CopyMemory(ssQuadGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	ssQuadGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(Device.Get(),
		CommandList.Get(), vertices.data(), vbByteSize, ssQuadGeo->VertexBufferUploader);

	ssQuadGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(Device.Get(),
		CommandList.Get(), indices.data(), ibByteSize, ssQuadGeo->IndexBufferUploader);

	ssQuadGeo->VertexByteStride = sizeof(Vertex);
	ssQuadGeo->VertexBufferByteSize = vbByteSize;
	ssQuadGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	ssQuadGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	ssQuadGeo->DrawArgs["quad01"] = submesh;
	Geometries["ssQuad"] = std::move(ssQuadGeo);
}

void Game::BuildRenderItems()
{
	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(10.0f, 10.0f, 10.0f) * XMMatrixTranslation(0.0f, 5.0f, 0.0f));
	boxRitem->ObjCBIndex = 0;
	boxRitem->Geo = Geometries["shapeGeo"].get();
	boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	AllRitems.push_back(std::move(boxRitem));

	auto gridRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&gridRitem->World, XMMatrixScaling(5.0f, 1.0f, 5.0f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f));
	gridRitem->ObjCBIndex = 1;
	gridRitem->Geo = Geometries["shapeGeo"].get();
	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	AllRitems.push_back(std::move(gridRitem));

	XMMATRIX brickTexTransform = XMMatrixScaling(10.0f, 10.0f, 10.0f);
	UINT objCBIndex = 2;
	for (int i = 0; i < 5; ++i)
	{
		auto leftCylRitem = std::make_unique<RenderItem>();
		auto rightCylRitem = std::make_unique<RenderItem>();
		auto leftSphereRitem = std::make_unique<RenderItem>();
		auto rightSphereRitem = std::make_unique<RenderItem>();

		XMMATRIX leftCylWorld = XMMatrixScaling(5.0f, 5.0f, 5.0f) * XMMatrixTranslation(-25.0f, 1.5f * 5, -50.0f + i * 25.0f);
		XMMATRIX rightCylWorld = XMMatrixScaling(5.0f, 5.0f, 5.0f) * XMMatrixTranslation(+25.0f, 1.5f * 5, -50.0f + i * 25.0f);

		XMMATRIX leftSphereWorld = XMMatrixScaling(5.0f, 5.0f, 5.0f) * XMMatrixTranslation(-25.0f, 3.5f * 5, -50.0f + i * 25.0f);
		XMMATRIX rightSphereWorld = XMMatrixScaling(5.0f, 5.0f, 5.0f) * XMMatrixTranslation(+25.0f, 3.5f * 5, -50.0f + i * 25.0f);

		XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
		XMStoreFloat4x4(&leftCylRitem->TexTransform, brickTexTransform);
		leftCylRitem->ObjCBIndex = objCBIndex++;
		leftCylRitem->Geo = Geometries["shapeGeo"].get();
		leftCylRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
		XMStoreFloat4x4(&rightCylRitem->TexTransform, brickTexTransform);
		rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Geo = Geometries["shapeGeo"].get();
		rightCylRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
		leftSphereRitem->TexTransform = MathHelper::Identity4x4();
		leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Geo = Geometries["shapeGeo"].get();
		leftSphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
		rightSphereRitem->TexTransform = MathHelper::Identity4x4();
		rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Geo = Geometries["shapeGeo"].get();
		rightSphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		AllRitems.push_back(std::move(leftCylRitem));
		AllRitems.push_back(std::move(rightCylRitem));
		AllRitems.push_back(std::move(leftSphereRitem));
		AllRitems.push_back(std::move(rightSphereRitem));
	}

	for (auto& e : AllRitems)
		OpaqueRitems.push_back(e.get());

	auto ssQuadRitem = std::make_unique<RenderItem>();
	ssQuadRitem->ObjCBIndex = objCBIndex++;
	ssQuadRitem->Geo = Geometries["ssQuad"].get();
	ssQuadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ssQuadRitem->IndexCount = ssQuadRitem->Geo->DrawArgs["quad01"].IndexCount;
	ssQuadRitem->StartIndexLocation = ssQuadRitem->Geo->DrawArgs["quad01"].StartIndexLocation;
	ssQuadRitem->BaseVertexLocation = ssQuadRitem->Geo->DrawArgs["quad01"].BaseVertexLocation;
	AllRitems.push_back(std::move(ssQuadRitem));

	SSRitems.push_back(AllRitems.back().get());
}

void Game::BuildUAVs()
{
	// G Buffer Textures
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(SRVUAVHeap->GetCPUDescriptorHandleForHeapStart(), 6, CBVSRVUAVDescriptorSize);
		D3D12_SHADER_RESOURCE_VIEW_DESC descSRV;

		ZeroMemory(&descSRV, sizeof(descSRV));
		descSRV.Texture2D.MipLevels = 1;
		descSRV.Texture2D.MostDetailedMip = 0;
		descSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		for (int i = 0; i < GBufferCount; i++) {
			descSRV.Format = GBufferFormats[i];
			Device->CreateShaderResourceView(GBuffer[i].Get(), &descSRV, hDescriptor);
			hDescriptor.Offset(1, CBVSRVUAVDescriptorSize);
		}

		Device->CreateShaderResourceView(AccumulationBuffer.Get(), &descSRV, hDescriptor);

		GBufferGPUSRV = CD3DX12_GPU_DESCRIPTOR_HANDLE(SRVUAVHeap->GetGPUDescriptorHandleForHeapStart(), 6, CBVSRVUAVDescriptorSize);
	}

	// Particle Pool
	{
		UINT64 particlePoolByteSize = sizeof(Particle) * emitter->GetMaxParticles();
		ThrowIfFailed(Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(particlePoolByteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&RWParticlePool)));
		RWParticlePool->SetName(L"ParticlePool");

		D3D12_UNORDERED_ACCESS_VIEW_DESC particlePoolUAVDescription = {};

		particlePoolUAVDescription.Format = DXGI_FORMAT_UNKNOWN;
		particlePoolUAVDescription.Buffer.FirstElement = 0;
		particlePoolUAVDescription.Buffer.NumElements = emitter->GetMaxParticles();
		particlePoolUAVDescription.Buffer.StructureByteStride = sizeof(Particle);
		particlePoolUAVDescription.Buffer.CounterOffsetInBytes = 0;
		particlePoolUAVDescription.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

		D3D12_SHADER_RESOURCE_VIEW_DESC particlePoolSRVDescription = {};
		particlePoolSRVDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		particlePoolSRVDescription.Format = DXGI_FORMAT_UNKNOWN;
		particlePoolSRVDescription.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		particlePoolSRVDescription.Buffer.FirstElement = 0;
		particlePoolSRVDescription.Buffer.NumElements = emitter->GetMaxParticles();
		particlePoolSRVDescription.Buffer.StructureByteStride = sizeof(Particle);

		ParticlePoolCPUUAV = CD3DX12_CPU_DESCRIPTOR_HANDLE(SRVUAVHeap->GetCPUDescriptorHandleForHeapStart(), 0, CBVSRVUAVDescriptorSize);
		ParticlePoolGPUUAV = CD3DX12_GPU_DESCRIPTOR_HANDLE(SRVUAVHeap->GetGPUDescriptorHandleForHeapStart(), 0, CBVSRVUAVDescriptorSize);
		Device->CreateUnorderedAccessView(RWParticlePool.Get(), nullptr, &particlePoolUAVDescription, ParticlePoolCPUUAV);

		ParticlePoolCPUSRV = CD3DX12_CPU_DESCRIPTOR_HANDLE(SRVUAVHeap->GetCPUDescriptorHandleForHeapStart(), 4, CBVSRVUAVDescriptorSize);
		ParticlePoolGPUSRV = CD3DX12_GPU_DESCRIPTOR_HANDLE(SRVUAVHeap->GetGPUDescriptorHandleForHeapStart(), 4, CBVSRVUAVDescriptorSize);
		Device->CreateShaderResourceView(RWParticlePool.Get(), &particlePoolSRVDescription, ParticlePoolCPUSRV);
	}

	// Dead List
	{
		UINT64 deadListByteSize = sizeof(unsigned int) * emitter->GetMaxParticles();
		UINT64 countBufferOffset = AlignForUavCounter(deadListByteSize);

		ThrowIfFailed(Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(countBufferOffset + sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&ACDeadList)));
		ACDeadList->SetName(L"ACDeadList");

		D3D12_UNORDERED_ACCESS_VIEW_DESC deadListUAVDescription = {};
		deadListUAVDescription.Format = DXGI_FORMAT_UNKNOWN;
		deadListUAVDescription.Buffer.FirstElement = 0;
		deadListUAVDescription.Buffer.NumElements = emitter->GetMaxParticles();
		deadListUAVDescription.Buffer.StructureByteStride = sizeof(unsigned	int);
		deadListUAVDescription.Buffer.CounterOffsetInBytes = countBufferOffset;
		deadListUAVDescription.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

		ACDeadListCPUUAV = CD3DX12_CPU_DESCRIPTOR_HANDLE(SRVUAVHeap->GetCPUDescriptorHandleForHeapStart(), 1, CBVSRVUAVDescriptorSize);
		ACDeadListGPUUAV = CD3DX12_GPU_DESCRIPTOR_HANDLE(SRVUAVHeap->GetGPUDescriptorHandleForHeapStart(), 1, CBVSRVUAVDescriptorSize);
		Device->CreateUnorderedAccessView(ACDeadList.Get(), ACDeadList.Get(), &deadListUAVDescription, ACDeadListCPUUAV);
	}

	// Draw List
	{
		UINT64 drawListByteSize = sizeof(ParticleSort) * emitter->GetMaxParticles();
		UINT64 countBufferOffset = AlignForUavCounter(drawListByteSize);

		ThrowIfFailed(Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(countBufferOffset + sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&RWDrawList)));
		RWDrawList->SetName(L"DrawList");

		D3D12_UNORDERED_ACCESS_VIEW_DESC drawlistUAVDescription = {};
		drawlistUAVDescription.Format = DXGI_FORMAT_UNKNOWN;
		drawlistUAVDescription.Buffer.FirstElement = 0;
		drawlistUAVDescription.Buffer.NumElements = emitter->GetMaxParticles();
		drawlistUAVDescription.Buffer.StructureByteStride = sizeof(ParticleSort);
		drawlistUAVDescription.Buffer.CounterOffsetInBytes = countBufferOffset;
		drawlistUAVDescription.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		drawlistUAVDescription.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

		DrawListCPUUAV = CD3DX12_CPU_DESCRIPTOR_HANDLE(SRVUAVHeap->GetCPUDescriptorHandleForHeapStart(), 2, CBVSRVUAVDescriptorSize);
		DrawListGPUUAV = CD3DX12_GPU_DESCRIPTOR_HANDLE(SRVUAVHeap->GetGPUDescriptorHandleForHeapStart(), 2, CBVSRVUAVDescriptorSize);
		Device->CreateUnorderedAccessView(RWDrawList.Get(), RWDrawList.Get(), &drawlistUAVDescription, DrawListCPUUAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC drawListSRVDescription = {};
		drawListSRVDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		drawListSRVDescription.Format = DXGI_FORMAT_UNKNOWN;
		drawListSRVDescription.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		drawListSRVDescription.Buffer.FirstElement = 0;
		drawListSRVDescription.Buffer.NumElements = emitter->GetMaxParticles();
		drawListSRVDescription.Buffer.StructureByteStride = sizeof(ParticleSort);

		DrawListCPUSRV = CD3DX12_CPU_DESCRIPTOR_HANDLE(SRVUAVHeap->GetCPUDescriptorHandleForHeapStart(), 5, CBVSRVUAVDescriptorSize);
		DrawListGPUSRV = CD3DX12_GPU_DESCRIPTOR_HANDLE(SRVUAVHeap->GetGPUDescriptorHandleForHeapStart(), 5, CBVSRVUAVDescriptorSize);
		Device->CreateShaderResourceView(RWDrawList.Get(), &drawListSRVDescription, DrawListCPUSRV);

		ThrowIfFailed(Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(countBufferOffset + sizeof(UINT)),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&DrawListUploadBuffer)));
	}

	// Draw Args
	{
		UINT64 drawArgsByteSize = (sizeof(unsigned int) * 9);
		UINT64 countBufferOffset = AlignForUavCounter(drawArgsByteSize);

		ThrowIfFailed(Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(countBufferOffset + sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&RWDrawArgs)));
		RWDrawArgs.Get()->SetName(L"DrawArgs");

		D3D12_UNORDERED_ACCESS_VIEW_DESC drawArgsUAVDescription = {};

		drawArgsUAVDescription.Format = DXGI_FORMAT_UNKNOWN;
		drawArgsUAVDescription.Buffer.FirstElement = 0;
		drawArgsUAVDescription.Buffer.NumElements = 9;
		drawArgsUAVDescription.Buffer.StructureByteStride = sizeof(unsigned int);
		drawArgsUAVDescription.Buffer.CounterOffsetInBytes = countBufferOffset;
		drawArgsUAVDescription.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		drawArgsUAVDescription.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

		DrawArgsCPUUAV = CD3DX12_CPU_DESCRIPTOR_HANDLE(SRVUAVHeap->GetCPUDescriptorHandleForHeapStart(), 3, CBVSRVUAVDescriptorSize);
		DrawArgsGPUUAV = CD3DX12_GPU_DESCRIPTOR_HANDLE(SRVUAVHeap->GetGPUDescriptorHandleForHeapStart(), 3, CBVSRVUAVDescriptorSize);
		Device->CreateUnorderedAccessView(RWDrawArgs.Get(), RWDrawArgs.Get(), &drawArgsUAVDescription, DrawArgsCPUUAV);
	}
}

void Game::BuildRootSignature()
{
	// geo root signature
	{
		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[1];
		slotRootParameter[0].InitAsConstantBufferView(0);

		auto staticSamplers = GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter,
			(UINT)staticSamplers.size(),
			staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(Device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(geoRootSignature.GetAddressOf())));
	}

	// g buffer root signature
	{
		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[1];

		CD3DX12_DESCRIPTOR_RANGE srvTable0;
		srvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

		slotRootParameter[0].InitAsDescriptorTable(1, &srvTable0);

		auto staticSamplers = GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter,
			(UINT)staticSamplers.size(),
			staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(Device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(gBufferRootSignature.GetAddressOf())));
	}

	// default root signature
	{
		CD3DX12_DESCRIPTOR_RANGE srvTable0;
		srvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		CD3DX12_DESCRIPTOR_RANGE srvTable1;
		srvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[5];

		// Create root CBVs.
		slotRootParameter[0].InitAsConstantBufferView(0);
		slotRootParameter[1].InitAsConstantBufferView(1);
		slotRootParameter[2].InitAsConstantBufferView(2);
		slotRootParameter[3].InitAsDescriptorTable(1, &srvTable0);
		slotRootParameter[4].InitAsDescriptorTable(1, &srvTable1);

		auto staticSamplers = GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter,
			(UINT)staticSamplers.size(),
			staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(Device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(rootSignature.GetAddressOf())));
	}

	// particle root signature
	{
		CD3DX12_DESCRIPTOR_RANGE uavTable0;
		uavTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

		CD3DX12_DESCRIPTOR_RANGE uavTable1;
		uavTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);

		CD3DX12_DESCRIPTOR_RANGE uavTable2;
		uavTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);

		CD3DX12_DESCRIPTOR_RANGE uavTable3;
		uavTable3.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 3);

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[7];

		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsConstantBufferView(0);
		slotRootParameter[1].InitAsConstantBufferView(1);
		slotRootParameter[2].InitAsConstantBufferView(2);
		slotRootParameter[3].InitAsDescriptorTable(1, &uavTable0);
		slotRootParameter[4].InitAsDescriptorTable(1, &uavTable1);
		slotRootParameter[5].InitAsDescriptorTable(1, &uavTable2);
		slotRootParameter[6].InitAsDescriptorTable(1, &uavTable3);

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(7, slotRootParameter,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(Device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(particleRootSignature.GetAddressOf())));
	}

	// particle command signature
	D3D12_INDIRECT_ARGUMENT_DESC Args[1];
	Args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

	D3D12_COMMAND_SIGNATURE_DESC particleCommandSingatureDescription = {};
	particleCommandSingatureDescription.ByteStride = 36;
	particleCommandSingatureDescription.NumArgumentDescs = 1;
	particleCommandSingatureDescription.pArgumentDescs = Args;

	ThrowIfFailed(Device->CreateCommandSignature(
		&particleCommandSingatureDescription,
		NULL,
		IID_PPV_ARGS(particleCommandSignature.GetAddressOf())));
}

void Game::BuildShadersAndInputLayout()
{
	Shaders["GeoOpaqueVS"] = d3dUtil::CompileShader(L"DefaultVS.hlsl", nullptr, "main", "vs_5_1");
	Shaders["GeoOpaquePS"] = d3dUtil::CompileShader(L"DefaultPS.hlsl", nullptr, "main", "ps_5_1");
	Shaders["AccumulationPassVS"] = d3dUtil::CompileShader(L"AccumulationPassVS.hlsl", nullptr, "main", "vs_5_1");
	Shaders["AccumulationPassPS"] = d3dUtil::CompileShader(L"AccumulationPassPS.hlsl", nullptr, "main", "ps_5_1");
	Shaders["RenderQuadVS"] = d3dUtil::CompileShader(L"RenderQuadVS.hlsl", nullptr, "main", "vs_5_1");
	Shaders["RenderQuadPS"] = d3dUtil::CompileShader(L"RenderQuadPS.hlsl", nullptr, "main", "ps_5_1");
	Shaders["VS"] = d3dUtil::CompileShader(L"ParticleVertexShader.hlsl", nullptr, "main", "vs_5_1");
	Shaders["GS"] = d3dUtil::CompileShader(L"ParticleGeometryShader.hlsl", nullptr, "main", "gs_5_1");
	Shaders["PS"] = d3dUtil::CompileShader(L"ParticlePixelShader.hlsl", nullptr, "main", "ps_5_1");
	Shaders["EmitCS"] = d3dUtil::CompileShader(L"EmitComputeShader.hlsl", nullptr, "main", "cs_5_1");
	Shaders["UpdateCS"] = d3dUtil::CompileShader(L"UpdateComputeShader.hlsl", nullptr, "main", "cs_5_1");
	Shaders["CopyDrawCountCS"] = d3dUtil::CompileShader(L"CopyDrawCountComputeShader.hlsl", nullptr, "main", "cs_5_1");
	Shaders["DeadListInitCS"] = d3dUtil::CompileShader(L"DeadListInitComputeShader.hlsl", nullptr, "main", "cs_5_1");

	geoInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void Game::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC geoOpaquePsoDesc;
	ZeroMemory(&geoOpaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	geoOpaquePsoDesc.InputLayout = { geoInputLayout.data(), (UINT)geoInputLayout.size() };
	geoOpaquePsoDesc.pRootSignature = geoRootSignature.Get();
	geoOpaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(Shaders["GeoOpaqueVS"]->GetBufferPointer()),
		Shaders["GeoOpaqueVS"]->GetBufferSize()
	};
	geoOpaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(Shaders["GeoOpaquePS"]->GetBufferPointer()),
		Shaders["GeoOpaquePS"]->GetBufferSize()
	};
	geoOpaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	geoOpaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	geoOpaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	geoOpaquePsoDesc.SampleMask = UINT_MAX;
	geoOpaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	geoOpaquePsoDesc.NumRenderTargets = GBufferCount;
	for (int i = 0; i < GBufferCount; i++)
		geoOpaquePsoDesc.RTVFormats[i] = GBufferFormats[i];
	geoOpaquePsoDesc.SampleDesc.Count = xMsaaState ? 4 : 1;
	geoOpaquePsoDesc.SampleDesc.Quality = xMsaaState ? (xMsaaQuality - 1) : 0;
	geoOpaquePsoDesc.DSVFormat = DepthStencilFormat;
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&geoOpaquePsoDesc, IID_PPV_ARGS(&PSOs["geoOpaque"])));


	D3D12_GRAPHICS_PIPELINE_STATE_DESC accumulationPsoDesc;
	ZeroMemory(&accumulationPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	accumulationPsoDesc.InputLayout = { geoInputLayout.data(), (UINT)geoInputLayout.size() };
	accumulationPsoDesc.pRootSignature = gBufferRootSignature.Get();
	accumulationPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(Shaders["AccumulationPassVS"]->GetBufferPointer()),
		Shaders["AccumulationPassVS"]->GetBufferSize()
	};
	accumulationPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(Shaders["AccumulationPassPS"]->GetBufferPointer()),
		Shaders["AccumulationPassPS"]->GetBufferSize()
	};
	accumulationPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	accumulationPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	accumulationPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	accumulationPsoDesc.SampleMask = UINT_MAX;
	accumulationPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	accumulationPsoDesc.NumRenderTargets = 1;
	accumulationPsoDesc.RTVFormats[0] = BackBufferFormat;
	accumulationPsoDesc.SampleDesc.Count = xMsaaState ? 4 : 1;
	accumulationPsoDesc.SampleDesc.Quality = xMsaaState ? (xMsaaQuality - 1) : 0;
	accumulationPsoDesc.DSVFormat = DepthStencilFormat;
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&accumulationPsoDesc, IID_PPV_ARGS(&PSOs["accumulationPass"])));
	
	accumulationPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(Shaders["RenderQuadVS"]->GetBufferPointer()),
		Shaders["RenderQuadVS"]->GetBufferSize()
	};
	accumulationPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(Shaders["RenderQuadPS"]->GetBufferPointer()),
		Shaders["RenderQuadPS"]->GetBufferSize()
	};
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&accumulationPsoDesc, IID_PPV_ARGS(&PSOs["renderQuad"])));	


	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePSODescription;
	ZeroMemory(&opaquePSODescription, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePSODescription.pRootSignature = rootSignature.Get();
	opaquePSODescription.VS =
	{
		reinterpret_cast<BYTE*>(Shaders["VS"]->GetBufferPointer()),
		Shaders["VS"]->GetBufferSize()
	};
	opaquePSODescription.PS =
	{
		reinterpret_cast<BYTE*>(Shaders["PS"]->GetBufferPointer()),
		Shaders["PS"]->GetBufferSize()
	};
	opaquePSODescription.GS =
	{
		reinterpret_cast<BYTE*>(Shaders["GS"]->GetBufferPointer()),
		Shaders["GS"]->GetBufferSize()
	};

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc = {};
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_MAX;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_ONE;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_DEPTH_STENCIL_DESC depth = {};
	depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depth.DepthEnable = true;

	opaquePSODescription.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePSODescription.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaquePSODescription.BlendState.RenderTarget[0] = transparencyBlendDesc;
	opaquePSODescription.DepthStencilState = depth;
	opaquePSODescription.SampleMask = UINT_MAX;
	opaquePSODescription.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	opaquePSODescription.NumRenderTargets = GBufferCount;
	for (int i = 0; i < GBufferCount; i++)
		opaquePSODescription.RTVFormats[i] = GBufferFormats[i];
	opaquePSODescription.SampleDesc.Count = xMsaaState ? 4 : 1;
	opaquePSODescription.SampleDesc.Quality = xMsaaState ? (xMsaaQuality - 1) : 0;
	opaquePSODescription.DSVFormat = DepthStencilFormat;
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&opaquePSODescription, IID_PPV_ARGS(&PSOs["opaque"])));

	D3D12_COMPUTE_PIPELINE_STATE_DESC particleEmitPSO = {};
	particleEmitPSO.pRootSignature = particleRootSignature.Get();
	particleEmitPSO.CS =
	{
		reinterpret_cast<BYTE*>(Shaders["EmitCS"]->GetBufferPointer()),
		Shaders["EmitCS"]->GetBufferSize()
	};
	particleEmitPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(Device->CreateComputePipelineState(&particleEmitPSO, IID_PPV_ARGS(&PSOs["particleEmit"])));

	D3D12_COMPUTE_PIPELINE_STATE_DESC particleUpdatePSO = {};
	particleUpdatePSO.pRootSignature = particleRootSignature.Get();
	particleUpdatePSO.CS =
	{
		reinterpret_cast<BYTE*>(Shaders["UpdateCS"]->GetBufferPointer()),
		Shaders["UpdateCS"]->GetBufferSize()
	};
	particleUpdatePSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(Device->CreateComputePipelineState(&particleUpdatePSO, IID_PPV_ARGS(&PSOs["particleUpdate"])));

	D3D12_COMPUTE_PIPELINE_STATE_DESC particleDrawPSO = {};
	particleDrawPSO.pRootSignature = particleRootSignature.Get();
	particleDrawPSO.CS =
	{
		reinterpret_cast<BYTE*>(Shaders["CopyDrawCountCS"]->GetBufferPointer()),
		Shaders["CopyDrawCountCS"]->GetBufferSize()
	};
	particleDrawPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(Device->CreateComputePipelineState(&particleDrawPSO, IID_PPV_ARGS(&PSOs["particleDraw"])));

	D3D12_COMPUTE_PIPELINE_STATE_DESC particleDeadListPSO = {};
	particleDeadListPSO.pRootSignature = particleRootSignature.Get();
	particleDeadListPSO.CS =
	{
		reinterpret_cast<BYTE*>(Shaders["DeadListInitCS"]->GetBufferPointer()),
		Shaders["DeadListInitCS"]->GetBufferSize()
	};
	particleDeadListPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(Device->CreateComputePipelineState(&particleDeadListPSO, IID_PPV_ARGS(&PSOs["particleDeadList"])));
}

void Game::BuildFrameResources()
{
	for (int i = 0; i < gNumberFrameResources; ++i)
	{
		FrameResources.push_back(std::make_unique<FrameResource>(Device.Get(),
			1, 1, 1));
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Game::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}