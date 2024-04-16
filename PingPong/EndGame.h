#pragma once
#include <iostream>
#include <windows.h>
#include <map>
#include "PlayerRenderComponent.h"
#include "BallRenderComponent.h"

class EndGame : public RenderComponent
{
public:
	EndGame(std::vector<PlayerRenderComponent*> players, BallRenderComponent* ball);
	void Build() override;
	void Update(const GameTimer& t, DirectX::XMFLOAT4X4 mProj) override;
	void Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList) override;
	void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
	void Finish(bool left);
	void LogScore();

	std::vector<PlayerRenderComponent*> mPlayers;
	int mScore[2];
	BallRenderComponent* mBall;
};

