#pragma once

using namespace System;
using namespace tesseract;

namespace TextRecognizer {
	public ref class TextRecognizingManager
	{
	private:
		TessBaseAPI* _api = NULL;

	public:
		HRESULT Recognize(array<byte>^ bytes, double x, double y, double width, double height);
		HRESULT Initialize();
		HRESULT Uninitialize();
	};
}
