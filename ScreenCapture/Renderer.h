#pragma once

namespace ScreenCapture
{
	class Renderer
	{
	public:
		virtual ~Renderer();

		HRESULT CheckDeviceState();
		HRESULT CreateSurface(UINT width, UINT height, bool useAlpha, UINT samplesCount);

		virtual HRESULT Render() = 0;

		IDirect3DSurface9* GetSurfaceNoRef();

	protected:
		Renderer();

		virtual HRESULT Init(IDirect3D9* d3d, IDirect3D9Ex* d3dEx, HWND hwnd, UINT adapter);

		IDirect3DDevice9* _device = NULL;
		IDirect3DDevice9Ex* _deviceEx = NULL;
		IDirect3DSurface9* _surface = NULL;
	};
}