#pragma once

#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "RenderComponent.h"

class RenderComponentsApp : public D3DApp
{
public:
    RenderComponentsApp(GameWindow* gameWindow);
    RenderComponentsApp(const RenderComponentsApp& rhs) = delete;
    RenderComponentsApp& operator=(const RenderComponentsApp& rhs) = delete;
    ~RenderComponentsApp();

    virtual bool Initialize()override;
    virtual void Handle(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

    void AddComponent(RenderComponent* component);
    void RemoveComponent(RenderComponent* component);
private:
    virtual void Resize()override;
    virtual void OnUpdate(const GameTimer& gt)override;
    virtual void OnDraw(const GameTimer& gt)override;

private:
    DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();
    std::vector<RenderComponent*> mComponents;
};