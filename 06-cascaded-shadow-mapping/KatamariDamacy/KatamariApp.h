#pragma once

#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "../Common/Camera.h"
#include "RenderComponent.h"
#include "Mesh.h"
#include "ShadowMap.h"


class KatamariApp : public D3DApp
{
public:
    KatamariApp(GameWindow* gameWindow);
    KatamariApp(const KatamariApp& rhs) = delete;
    KatamariApp& operator=(const KatamariApp& rhs) = delete;
    ~KatamariApp();

    virtual bool Initialize()override;
    virtual void Handle(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
    void InitShadowMap();

    void AddComponent(RenderComponent* component);
    void AddSSComponent(RenderComponent* component);
    void RemoveComponent(RenderComponent* component);
    Camera* GetMainCamera();
    std::vector<ShadowMap*> GetShadowMap();

    ID3D12InfoQueue* infoQueue;
private:
    virtual void Resize()override;
    virtual void OnUpdate(const GameTimer& gt)override;
    virtual void OnDraw(const GameTimer& gt)override;

private:
    std::vector<RenderComponent*> mComponents;
    std::vector<RenderComponent*> mSSComponents;
    Camera mCamera;
    POINT mLastMousePos;
    std::vector<std::unique_ptr<ShadowMap>> mShadowMaps;
    Shader* mShadowShader;
};