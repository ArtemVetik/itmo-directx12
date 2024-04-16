#include "EndGame.h"

EndGame::EndGame(std::vector<PlayerRenderComponent*> players, BallRenderComponent* ball)
{
	mPlayers = players;
	mBall = ball;
	mScore[0] = mScore[1] = 0;
}

void EndGame::Finish(bool left)
{
	for (auto& player : mPlayers)
		player->Reset();

	mBall->Reset();

	mScore[left ? 1 : 0]++;
	LogScore();
}

void EndGame::LogScore()
{
	int index = 1;
	
	for (auto& score : mScore)
		std::cout << "Player: " << index++ << ": " << score << " score\n";

	std::cout << "------------------------------------------\n";
}

void EndGame::Build()
{

}

void EndGame::Update(const GameTimer& t, DirectX::XMFLOAT4X4 mProj)
{
	float xPos = mBall->GetPosition().x;

	if (abs(xPos) >= 1.0f)
		Finish(xPos <= -1.0f);
}

void EndGame::Draw(const GameTimer& t, ID3D12GraphicsCommandList* commandList)
{
}

void EndGame::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
}
