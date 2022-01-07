#pragma once
#include "ISurfaceQueue.h"
#include <cliext/vector>
#include "RenderOutput.h"

using namespace System::Windows::Interop;
using namespace cliext;

namespace ScreenCapture {
	public ref class CaptureManager
	{
	public:
		CaptureManager();
		~CaptureManager();

		void Render();
		RenderOutput^ AddOutput(D3DImage^ d3dImage, Rect^ window);

	private:
		HRESULT CaptureManager::InitializeD3D11();

	private:
		ID3D11Device* _device = nullptr;
		ID3D11DeviceContext* _deviceContext = nullptr;
		IDXGIOutputDuplication* _outputDuplication = nullptr;
		ID3D11Buffer* _vertexBuffer = nullptr;

		Size^ _screenSize = nullptr;

		vector<RenderOutput^>^ _outputs = nullptr;
	};
}
