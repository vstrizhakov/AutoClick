#pragma once

namespace ScreenCapture
{
	class Renderer;

	class RenderingManager
	{
	public:
		static HRESULT Create(RenderingManager** manager);
		~RenderingManager();

		void SetSize(UINT width, UINT height);
		void SetAlpha(bool useAlpha);
		void SetDesiredSampledCount(UINT numSamples);
		void SetAdapter(POINT screenSpacePoint);

		int GetHeight();
		int GetWidth();
		bool GetAlpha();
		int GetDesiredSampledCount();

		HRESULT GetBackBufferNoRef(IDirect3DSurface9** surface);
		HRESULT Render();

	private:
		RenderingManager();

		void CleanupInvalidDevices();
		HRESULT EnsureRenderers();
		HRESULT EnsureHWND();
		HRESULT EnsureD3DObjects();
		HRESULT TestSurfaceSettings();
		void DestroyResources();

		IDirect3D9* _d3d;
		IDirect3D9Ex* _d3dEx;

		UINT _adaptersCount;
		Renderer** _renderers;
		Renderer* _currentRenderer;

		HWND _hwnd;

		UINT _width;
		UINT _height;
		UINT _desiredSamplesCount;
		bool _useAlpha;
		bool _surfaceSettingsChanged;
	};
}

