#include <iostream>
#include <Windows.h>
#include "PlanetsApp.h"
#include "WorldGrid.h"
#include "WorldGridMesh.h"
#include "PlanetRenderComponent.h"
#include "CameraRootController.h"
#include "SphereMesh.h"

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
		PlanetsApp theApp(mainWnd);

		if (!theApp.Initialize())
			return 0;

		WorldGridMesh gridMesh(theApp.GetDevice(), theApp.GetCommandList());
		theApp.BuildMesh(&gridMesh);
		Shader gridShader(theApp.GetDevice());
		gridShader.Initialize();
		WorldGrid grid(&gridMesh, &gridShader);
		theApp.AddComponent(&grid);

		SphereMesh sphereMesh(theApp.GetDevice(), theApp.GetCommandList());
		theApp.BuildMesh(&sphereMesh);

		Shader sunShader(theApp.GetDevice());
		sunShader.Initialize();
		PointPlanetRoot zeroRoot({});
		PlanetSettings sunSettings;
		sunSettings.Root = &zeroRoot;
		sunSettings.RotateRadius = 0.0f;
		sunSettings.Scale = { 0.6f, 0.6f, 0.6f };
		sunSettings.RotateAroundSpeed = 0.0f;
		sunSettings.RotateSelfSpeed = 0.4f;
		PlanetRenderComponent sunPlanet(&sphereMesh, &sunShader, sunSettings);
		theApp.AddComponent(&sunPlanet);

		Shader earthShader(theApp.GetDevice());
		earthShader.Initialize();
		PlanetSettings earthSettings;
		earthSettings.Root = &sunPlanet;
		earthSettings.RotateRadius = 4.0f;
		earthSettings.Scale = { 0.3f, 0.3f, 0.3f };
		earthSettings.RotateAroundSpeed = 0.2f;
		earthSettings.OrbitAngle = -0.1f;
		earthSettings.RotateSelfSpeed = 2.0f;
		earthSettings.Rotation = DirectX::XMQuaternionRotationRollPitchYaw(0.0f, 0, 0);
		PlanetRenderComponent earthPlanet(&sphereMesh, &earthShader, earthSettings);
		theApp.AddComponent(&earthPlanet);

		Shader moonShader(theApp.GetDevice());
		moonShader.Initialize();
		PlanetSettings moonSettings;
		moonSettings.Root = &earthPlanet;
		moonSettings.RotateRadius = 0.6f;
		moonSettings.Scale = { 0.1f, 0.1f, 0.1f };
		moonSettings.RotateAroundSpeed = 1.5f;
		moonSettings.RotateSelfSpeed = 2.5f;
		moonSettings.OrbitAngle = 0.4f;
		PlanetRenderComponent moonPlanet(&sphereMesh, &moonShader, moonSettings);
		theApp.AddComponent(&moonPlanet);

		Shader marsShader(theApp.GetDevice());
		marsShader.Initialize();
		PlanetSettings marsSettings;
		marsSettings.Root = &sunPlanet;
		marsSettings.RotateRadius = 8.0f;
		marsSettings.StartAngle = 1.1f;
		marsSettings.Scale = { 0.4f, 0.4f, 0.4f };
		marsSettings.RotateAroundSpeed = 0.4f;
		marsSettings.RotateSelfSpeed = 2.5f;
		marsSettings.OrbitAngle = 0.2f;
		PlanetRenderComponent marsPlanet(&sphereMesh, &marsShader, marsSettings);
		theApp.AddComponent(&marsPlanet);

		Shader marsMoon1Shader(theApp.GetDevice());
		marsMoon1Shader.Initialize();
		PlanetSettings marsMoon1Settings;
		marsMoon1Settings.Root = &marsPlanet;
		marsMoon1Settings.RotateRadius = 0.9f;
		marsMoon1Settings.StartAngle = 1.6f;
		marsMoon1Settings.Scale = { 0.1f, 0.1f, 0.1f };
		marsMoon1Settings.RotateAroundSpeed = 0.5f;
		marsMoon1Settings.RotateSelfSpeed = 2.5f;
		marsMoon1Settings.OrbitAngle = 0.0f;
		PlanetRenderComponent marsMoon1Planet(&sphereMesh, &marsMoon1Shader, marsMoon1Settings);
		theApp.AddComponent(&marsMoon1Planet);

		Shader marsMoon2Shader(theApp.GetDevice());
		marsMoon2Shader.Initialize();
		PlanetSettings marsMoon2Settings;
		marsMoon2Settings.Root = &marsPlanet;
		marsMoon2Settings.RotateRadius = 1.3f;
		marsMoon2Settings.StartAngle = 0.0f;
		marsMoon2Settings.Scale = { 0.08f, 0.08f, 0.08f };
		marsMoon2Settings.RotateAroundSpeed = 0.8f;
		marsMoon2Settings.RotateSelfSpeed = 2.5f;
		marsMoon2Settings.OrbitAngle = 0.1f;
		PlanetRenderComponent marsMoon2Planet(&sphereMesh, &marsMoon2Shader, marsMoon2Settings);
		theApp.AddComponent(&marsMoon2Planet);

		CameraRootController cameraRootController(&theApp, { &sunPlanet, &earthPlanet, &moonPlanet, &marsPlanet, &marsMoon1Planet, &marsMoon2Planet });
		theApp.AddComponent(&cameraRootController);

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