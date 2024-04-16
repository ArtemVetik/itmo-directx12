#pragma once

#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "RenderComponent.h"
#include "Mesh.h"

class PingPongApp : public D3DApp
{
public:
    PingPongApp(GameWindow* gameWindow);
    PingPongApp(const PingPongApp& rhs) = delete;
    PingPongApp& operator=(const PingPongApp& rhs) = delete;
    ~PingPongApp();

    virtual bool Initialize()override;
    virtual void Handle(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

    void BuildMesh(Mesh* mesh);
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