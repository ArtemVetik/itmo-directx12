#pragma once

#include <d3d12.h>

namespace Constants
{
	static const D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	static const DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static const DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	static bool m4xMsaaState = false;
	static UINT m4xMsaaQuality = 0;
}