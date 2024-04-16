#pragma once

#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "../Common/Camera.h"
#include "PlanetRenderComponent.h"
#include "Mesh.h"


class PlanetsApp : public D3DApp
{
public:
    PlanetsApp(GameWindow* gameWindow);
    PlanetsApp(const PlanetsApp& rhs) = delete;
    PlanetsApp& operator=(const PlanetsApp& rhs) = delete;
    ~PlanetsApp();

    virtual bool Initialize()override;
    virtual void Handle(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

    void BuildMesh(Mesh* mesh);
    void AddComponent(RenderComponent* component);
    void RemoveComponent(RenderComponent* component);
    void SetCameraRoot(PlanetRoot* root);
private:
    virtual void Resize()override;
    virtual void OnUpdate(const GameTimer& gt)override;
    virtual void OnDraw(const GameTimer& gt)override;

private:
    std::vector<RenderComponent*> mComponents;
    Camera mCamera;
    PlanetRoot* mCameraRoot;
    POINT mLastMousePos;
};