#include "Shader.h"

Shader::Shader(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::wstring shaderPath)
{
	mDevice = device;
	mCommandList = commandList;
	mShaderPath = shaderPath;
}

void Shader::Initialize()
{
	BuildRootSignature();
	BuildShadersAndInputLayout();
}

void Shader::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(mShaderPath, nullptr, "VS", "vs_5_1");
	mpsByteCode = d3dUtil::CompileShader(mShaderPath, nullptr, "PS", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers()
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

	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		6, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp,
		shadow
	};
}

void Shader::BuildRootSignature()
{
	// Shader programs typically require resources as input (constant buffers,
	// textures, samplers).  The root signature defines the resources the shader
	// programs expect.  If we think of the shader programs as a function, and
	// the input resources as function parameters, then the root signature can be
	// thought of as defining the function signature.  

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Create a single descriptor table of CBVs.
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1);

	CD3DX12_DESCRIPTOR_RANGE gBuf;
	gBuf.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 5);
	
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsDescriptorTable(1, &cbvTable1, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[3].InitAsDescriptorTable(1, &gBuf, D3D12_SHADER_VISIBILITY_ALL);

	auto a = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter, a.size(), a.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(mDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

D3D12_INPUT_LAYOUT_DESC Shader::GetInputLayout() const
{
	return { mInputLayout.data(), (UINT)mInputLayout.size() };
}

ID3D12RootSignature* Shader::GetRootSignature() const
{
	return mRootSignature.Get();
}

D3D12_SHADER_BYTECODE Shader::GetVS() const
{
	return {
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize()
	};
}

D3D12_SHADER_BYTECODE Shader::GetPS() const
{
	return {
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};
}
