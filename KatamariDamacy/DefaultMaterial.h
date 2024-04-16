#pragma once

#include <codecvt>
#include "Shader.h"
#include "KatamariApp.h"

class DefaultMaterial
{
public:
    DefaultMaterial(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Shader* shader, KatamariApp* app);
    
    void Initialize(std::string texturePath);
    void LoadTexture(std::string texturePath);
    void BuildDescriptorHeap();
    void BuildShaderResources();
    void BuildPSO();
    void SetRenderState(int cbOffset = 0);
    void CopyData(int elementIndex, const ObjectConstants& data);
    
private:
    ID3D12Device* mDevice;
    ID3D12GraphicsCommandList* mCommandList;
    Shader* mShader;
    KatamariApp* mApp;
    
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO = nullptr;
    std::unique_ptr<Texture> mTexture;
};
