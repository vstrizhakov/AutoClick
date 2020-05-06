#include "pch.h"
#include "Renderer.h"

using namespace ScreenCapture;

Renderer::Renderer() :
	_device(NULL),
	_deviceEx(NULL),
	_surface(NULL)
{

}

Renderer::~Renderer()
{
	SafeRelease(&_device);
	SafeRelease(&_deviceEx);
	SafeRelease(&_surface);
}

HRESULT Renderer::CheckDeviceState()
{
	HRESULT hResult = D3DERR_DEVICELOST;
	if (_device)
	{
		hResult = _device->TestCooperativeLevel();
	}
	else if (_deviceEx)
	{
		hResult = _deviceEx->CheckDeviceState(NULL);
	}
	return hResult;
}

HRESULT Renderer::CreateSurface(UINT width, UINT height, bool useAlpha, UINT samplesCount)
{
	HRESULT hResult = S_OK;

	SafeRelease(&_surface);

	hResult = _device->CreateRenderTarget(
		width,
		height,
		useAlpha ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8,
		static_cast<D3DMULTISAMPLE_TYPE>(samplesCount),
		0,
		_deviceEx ? FALSE : TRUE,
		&_surface,
		NULL);
	CHECKHR(hResult);

	hResult = _device->SetRenderTarget(0, _surface);
	CHECKHR(hResult);

CLEANUP:
	return hResult;
}


HRESULT Renderer::Init(IDirect3D9* d3d, IDirect3D9Ex* d3dEx, HWND hwnd, UINT adapter)
{
	HRESULT hResult = S_OK;

	D3DPRESENT_PARAMETERS presentParameters;
	ZeroMemory(&presentParameters, sizeof(presentParameters));
	presentParameters.Windowed = TRUE;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.BackBufferHeight = 1;
	presentParameters.BackBufferWidth = 1;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

	D3DCAPS9 caps;
	DWORD vertexProcessing;
	hResult = d3d->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &caps);
	CHECKHR(hResult);
	if ((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == D3DDEVCAPS_HWTRANSFORMANDLIGHT)
	{
		vertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	}
	else
	{
		vertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}

	if (d3dEx)
	{
		IDirect3DDevice9Ex* device = NULL;
		hResult = d3dEx->CreateDeviceEx(
			adapter,
			D3DDEVTYPE_HAL,
			hwnd,
			vertexProcessing | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
			&presentParameters,
			NULL,
			&_deviceEx);
		CHECKHR(hResult);

		hResult = _deviceEx->QueryInterface(__uuidof(IDirect3DDevice9), reinterpret_cast<void**>(&_device));
		CHECKHR(hResult);
	}
	else
	{
		assert(d3d);

		hResult = d3d->CreateDevice(
			adapter,
			D3DDEVTYPE_HAL,
			hwnd,
			vertexProcessing | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
			&presentParameters,
			&_device);
		CHECKHR(hResult);
	}

CLEANUP:
	return hResult;
}

IDirect3DSurface9* Renderer::GetSurfaceNoRef()
{
	return _surface;
}