#include <iostream>
#include <Windows.h>
#include "PingPongApp.h"
#include "PlayerRenderComponent.h"
#include "BallRenderComponent.h"
#include "RectangleMesh.h"
#include "CircleMesh.h"
#include "EndGame.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    FILE* fp;

    AllocConsole();
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);

    GameWindow* mainWnd = new GameWindow(hInstance);
    
    if (!mainWnd->Initialize())
        return -1;

    try
    {
        PingPongApp theApp(mainWnd);
        
        if (!theApp.Initialize())
            return 0;

        Shader firstShader(theApp.GetDevice());
        firstShader.Initialize();

        Shader secondShader(theApp.GetDevice());
        secondShader.Initialize();

        Shader ballShader(theApp.GetDevice());
        ballShader.Initialize();

        RectangleMesh rectMesh(theApp.GetDevice(), theApp.GetCommandList());
        theApp.BuildMesh(&rectMesh);

        CircleMesh circleMesh(theApp.GetDevice(), theApp.GetCommandList());
        theApp.BuildMesh(&circleMesh);

        PlayerRenderComponent firstPlayer(&rectMesh, &firstShader, { -0.8f, 0 }, { 'W', 'S'});
        firstPlayer.Build();

        PlayerRenderComponent secondPlayer(&rectMesh, &secondShader, { +0.8f, 0 }, { VK_UP, VK_DOWN });
        secondPlayer.Build();

        std::vector<PlayerRenderComponent*> players = { &firstPlayer, &secondPlayer };
        BallRenderComponent ball(&circleMesh, &ballShader, players);
        ball.Build();

        EndGame endGame(players, &ball);
        endGame.Build();

        theApp.AddComponent(&firstPlayer);
        theApp.AddComponent(&secondPlayer);
        theApp.AddComponent(&ball);
        theApp.AddComponent(&endGame);

        MSG msg = { 0 };

        while (msg.message != WM_QUIT)
        {
            msg = mainWnd->ExecuteMsg();

            if (mainWnd->IsPaused())
            {
                Sleep(100);
            }
            else
            {
                theApp.Update();
                theApp.Draw();
            }
        }

        return msg.wParam;
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }

    delete mainWnd;
}