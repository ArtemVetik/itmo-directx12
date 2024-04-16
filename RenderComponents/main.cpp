#include <Windows.h>
#include "RenderComponentsApp.h"
#include "BoxRenderComponent.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    GameWindow* mainWnd = new GameWindow(hInstance);
    
    if (!mainWnd->Initialize())
        return -1;

    try
    {
        RenderComponentsApp theApp(mainWnd);
        
        if (!theApp.Initialize())
            return 0;

        Shader s(theApp.GetDevice());
        s.Initialize();

        BoxRenderComponent boxRenderer(theApp.GetDevice(), theApp.GetCommandList(), &s, -1.0);
        boxRenderer.Build();

        BoxRenderComponent boxRenderer2(theApp.GetDevice(), theApp.GetCommandList(), &s, 1.0);
        boxRenderer2.Build();

        theApp.AddComponent(&boxRenderer);
        theApp.AddComponent(&boxRenderer2);

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