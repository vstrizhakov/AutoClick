#include "pch.h"

#include "TextRecognizingManager.h"

using namespace TextRecognizer;
using namespace System::Runtime::InteropServices;

HRESULT TextRecognizingManager::Initialize()
{
	_api = new TessBaseAPI();
	BOOL isInitializingFailed = _api->Init(NULL, "eng");
	return (isInitializingFailed) ? E_FAIL : S_OK;
}

HRESULT TextRecognizingManager::Uninitialize()
{
	if (_api)
	{
		_api->End();
	}
	return S_OK;
}

HRESULT TextRecognizingManager::Recognize(array<byte>^ bytes, double x, double y, double width, double height)
{
	if (!_api)
	{
		return E_FAIL;
	}

	l_uint32* data = new l_uint32[bytes->Length];
	for (int i = 0; i < bytes->Length; i++)
	{
		data[i] = bytes[i];
	}

	Pix* pix = pixCreate(1920, 1080, 32);
	pixSetData(pix, data);
	pixEndianByteSwap(pix);

	_api->SetImage(pix);
	_api->SetRectangle(x, y, width, height);
	char* outText = _api->GetUTF8Text();
	String^ str = gcnew String(outText);

	pixDestroy(&pix);
	return S_OK;
}
