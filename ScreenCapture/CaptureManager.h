#pragma once
#include "ISurfaceQueue.h"

using namespace System::Windows::Interop;

namespace ScreenCapture {
	public ref class CaptureManager
	{
	public:
		CaptureManager(int width, int height, bool useAlpha, int desiredSamplesCount);
		~CaptureManager();


		void Render(D3DImage^ d3dImage);

	private:
		HRESULT InitializeD3D9(UINT width, UINT height);
		HRESULT InitializeD3D11(UINT width, UINT height);
		HRESULT InitializeSurfaceQueue(UINT width, UINT height);

	private:

		ID3D11Device* _device = nullptr;
		ID3D11DeviceContext* _deviceContext = nullptr;
		/*ID3D11Texture2D* _texture = nullptr;*/
		ID3D11RenderTargetView* _renderTarget = nullptr;

		IDirect3D9Ex* _d3d9 = nullptr;
		IDirect3DDevice9Ex* _d3d9device = nullptr;
		IDXGIOutputDuplication* _outputDuplication = nullptr;
		ID3D11Buffer* _vertexBuffer = nullptr;

		ISurfaceQueue* _ABSurfaceQueue = nullptr;
		ISurfaceQueue* _BASurfaceQueue = nullptr;
		ISurfaceProducer* _ABSurfaceProducer = nullptr;
		ISurfaceConsumer* _ABSurfaceConsumer = nullptr;
		ISurfaceProducer* _BASurfaceProducer = nullptr;
		ISurfaceConsumer* _BASurfaceConsumer = nullptr;
	};
}
