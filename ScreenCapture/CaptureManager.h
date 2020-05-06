#pragma once

using namespace System::Windows::Interop;

namespace ScreenCapture {
	public ref class CaptureManager
	{
	public:
		CaptureManager(int width, int height, bool useAlpha, int desiredSamplesCount);
		~CaptureManager();

		property int Width
		{
			int get()
			{
				return _renderingManager->GetWidth();
			}
			void set(int width)
			{
				_renderingManager->SetSize(width, Height);
			}
		}
		property int Height
		{
			int get()
			{
				return _renderingManager->GetHeight();
			}
			void set(int height)
			{
				_renderingManager->SetSize(Width, height);
			}
		}
		property bool UseAlpha
		{
			bool get()
			{
				return _renderingManager->GetAlpha();
			}
			void set(bool useAlpha)
			{
				_renderingManager->SetAlpha(useAlpha);
			}
		}
		property int DesiredSamplesCount
		{
			int get()
			{
				return _renderingManager->GetDesiredSampledCount();
			}
			void set(int desiredSamplesCount)
			{
				_renderingManager->SetDesiredSampledCount(desiredSamplesCount);
			}
		}

		void Render(D3DImage^ d3dImage);

	private:
		RenderingManager* _renderingManager = NULL;
	};
}
