#include <iostream>
#include <Windows.h>
#include "DefaultMaterial.h"
#include "KatamariApp.h"
#include "WorldGrid.h"
#include "WorldGridMesh.h"
#include "KatamariObject.h"
#include "StaticObject.h"
#include "SphereMesh.h"
#include "FileMesh.h"
#include "Player.h"
#include "SSQuadMesh.h"
#include "SSQuad.h"

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
		KatamariApp theApp(mainWnd);

		if (!theApp.Initialize())
			return 0;

		theApp.BeginCommands();

		theApp.InitShadowMap();

		Shader defaultShader(theApp.GetDevice(), theApp.GetCommandList(), L"Shaders\\Default.hlsl");
		defaultShader.Initialize();

		Shader shadowDebug(theApp.GetDevice(), theApp.GetCommandList(), L"Shaders\\ShadowDebug.hlsl");
		shadowDebug.Initialize();

		WorldGridMesh gridMesh(theApp.GetDevice(), theApp.GetCommandList());
		gridMesh.Build();

		FileMesh floorMesh(theApp.GetDevice(), theApp.GetCommandList(), "Models/plane.fbx");
		floorMesh.Build();

		SphereMesh sphereMesh(theApp.GetDevice(), theApp.GetCommandList());
		sphereMesh.Build();

		FileMesh boxMesh(theApp.GetDevice(), theApp.GetCommandList(), "Models/box_low.obj");
		boxMesh.Build();

		FileMesh vaseMesh(theApp.GetDevice(), theApp.GetCommandList(), "Models/vase.obj");
		vaseMesh.Build();

		FileMesh cakeMesh(theApp.GetDevice(), theApp.GetCommandList(), "Models/Easter_cake.obj");
		cakeMesh.Build();

		FileMesh capybaraMesh(theApp.GetDevice(), theApp.GetCommandList(), "Models/Capybara.fbx");
		capybaraMesh.Build();

		FileMesh appleMesh(theApp.GetDevice(), theApp.GetCommandList(), "Models/Apple.fbx");
		appleMesh.Build();

		GeometryGenerator geoGen;
		SSQuadMesh ssQuadMesh1(theApp.GetDevice(), theApp.GetCommandList(), geoGen.CreateQuad(0.0f, -0.2f, 0.5f, 0.5f, 0.0f));
		ssQuadMesh1.Build();
		SSQuadMesh ssQuadMesh2(theApp.GetDevice(), theApp.GetCommandList(), geoGen.CreateQuad(-1.0f, -0.2f, 0.5f, 0.5f, 0.0f));
		ssQuadMesh2.Build();

		DefaultMaterial floorMaterial(theApp.GetDevice(), theApp.GetCommandList(), &defaultShader, &theApp);
		floorMaterial.Initialize("Models/Floortile1Color.dds");

		DefaultMaterial gridMaterial(theApp.GetDevice(), theApp.GetCommandList(), &defaultShader, &theApp);
		gridMaterial.Initialize("Models/DefaultMaterial_albedo.dds");

		WorldGrid grid(&gridMesh, &gridMaterial);

		DefaultMaterial boxMaterial(theApp.GetDevice(), theApp.GetCommandList(), &defaultShader, &theApp);
		boxMaterial.Initialize("Models/DefaultMaterial_albedo.dds");

		DefaultMaterial vaseMaterial(theApp.GetDevice(), theApp.GetCommandList(), &defaultShader, &theApp);
		vaseMaterial.Initialize("Models/vase_base_albedo.dds");

		DefaultMaterial cakeMaterial(theApp.GetDevice(), theApp.GetCommandList(), &defaultShader, &theApp);
		cakeMaterial.Initialize("Models/Pasha_LP_DefaultMaterial_BaseColor.dds");

		DefaultMaterial capybaraMaterial(theApp.GetDevice(), theApp.GetCommandList(), &defaultShader, &theApp);
		capybaraMaterial.Initialize("Models/4k_Capybara_V1_Diffuse.dds");

		DefaultMaterial appleMaterial(theApp.GetDevice(), theApp.GetCommandList(), &defaultShader, &theApp);
		appleMaterial.Initialize("Models/Apple_BaseColor.dds");

		DefaultMaterial quadMaterial1(theApp.GetDevice(), theApp.GetCommandList(), &shadowDebug, &theApp);
		quadMaterial1.Initialize("Models/4k_Capybara_V1_Diffuse.dds");
		DefaultMaterial quadMaterial2(theApp.GetDevice(), theApp.GetCommandList(), &shadowDebug, &theApp);
		quadMaterial2.Initialize("Models/4k_Capybara_V1_Diffuse.dds");

		SSQuad ssQuad1(&ssQuadMesh1, &quadMaterial1, 0);
		SSQuad ssQuad2(&ssQuadMesh2, &quadMaterial2, 1);

		StaticObject floor1(&floorMesh, &floorMaterial, &theApp, DirectX::XMMatrixAffineTransformation({ 0.12f, 0.12f, 0.12f }, {}, DirectX::XMQuaternionIdentity(), {0, -1, 0}));

		KatamariObjectSettings boxSettings{};
		boxSettings.Position = { -10, 0, 0 };
		boxSettings.Scale = { 0.003f, 0.003f, 0.003f };
		KatamariObject boxObject(&boxMesh, &boxMaterial, boxSettings, &theApp);

		KatamariObjectSettings vaseSettings{};
		vaseSettings.Position = { 0, 0, 10 };
		vaseSettings.Scale = { 0.01f, 0.01f, 0.01f };
		KatamariObject vaseObject(&vaseMesh, &vaseMaterial, vaseSettings, &theApp);

		KatamariObjectSettings cakeSettings{};
		cakeSettings.Position = { 0, 0, -10 };
		cakeSettings.Scale = { 0.005f, 0.005f, 0.005f };
		KatamariObject cakeObject(&cakeMesh, &cakeMaterial, cakeSettings, &theApp);

		KatamariObjectSettings capybaraSettings{};
		capybaraSettings.Position = { -5, 0, 8 };
		capybaraSettings.Scale = { 0.5f, 0.5f, 0.5f };
		KatamariObject capybaraObject(&capybaraMesh, &capybaraMaterial, capybaraSettings, &theApp);

		KatamariObjectSettings appleSettings{};
		appleSettings.Position = { 5, 0, -8 };
		appleSettings.Scale = { 1.5f, 1.5f, 1.5f };
		KatamariObject appleObject(&appleMesh, &appleMaterial, appleSettings, &theApp);

		DefaultMaterial playerMaterial(theApp.GetDevice(), theApp.GetCommandList(), &defaultShader, &theApp);
		playerMaterial.Initialize("Models/Default-Material_Paint-Layer-1.dds");

		FileMesh playerMesh(theApp.GetDevice(), theApp.GetCommandList(), "Models/Bolita_fuego_pintada_01.obj");
		playerMesh.Build();

		std::vector<KatamariObject*> objects = { &boxObject, &vaseObject, &cakeObject, &capybaraObject, &appleObject };
		Player player(theApp.GetMainCamera(), &playerMaterial, &playerMesh, objects);

		theApp.EndCommands();

		theApp.AddComponent(&floor1);
		//theApp.AddComponent(&grid);
		theApp.AddComponent(&boxObject);
		theApp.AddComponent(&vaseObject);
		theApp.AddComponent(&cakeObject);
		theApp.AddComponent(&capybaraObject);
		theApp.AddComponent(&appleObject);
		theApp.AddComponent(&player);
		theApp.AddSSComponent(&ssQuad1);
		theApp.AddSSComponent(&ssQuad2);

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