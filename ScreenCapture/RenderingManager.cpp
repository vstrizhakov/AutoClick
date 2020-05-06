#include "pch.h"
#include "OffScreenRenderer.h"
#include "RenderingManager.h"

using namespace ScreenCapture;

const static TCHAR AppName[] = TEXT("D3DImageSample");
typedef HRESULT(WINAPI* DIRECT3DCREATE9EXFUNCTION)(UINT sdkVersion, IDirect3D9Ex**);

RenderingManager::RenderingManager() :
	_d3d(NULL),
	_d3dEx(NULL),
	_adaptersCount(0),
	_hwnd(NULL),
	_currentRenderer(NULL),
	_renderers(NULL),
	_width(1024),
	_height(1024),
	_desiredSamplesCount(0),
	_useAlpha(false),
	_surfaceSettingsChanged(true)
{

}

RenderingManager::~RenderingManager()
{
	DestroyResources();
	if (_hwnd)
	{
		DestroyWindow(_hwnd);
		UnregisterClass(AppName, NULL);
	}
}

HRESULT RenderingManager::Create(RenderingManager** manager)
{
	HRESULT hResult = S_OK;
	*manager = new RenderingManager();

	CHECKOUTOFMEMORY(*manager);
	
CLEANUP:
	return hResult;
}

HRESULT RenderingManager::EnsureRenderers()
{
	HRESULT hResult = S_OK;
	if (!_renderers)
	{
		hResult = EnsureHWND();
		CHECKHR(hResult);

		assert(_adaptersCount);
		_renderers = new Renderer*[_adaptersCount];
		CHECKOUTOFMEMORY(_renderers);

		ZeroMemory(_renderers, _adaptersCount * sizeof(_renderers[0]));

		for (UINT i = 0; i < _adaptersCount; i++)
		{
			hResult = OffScreenRenderer::Create(_d3d, _d3dEx, _hwnd, i, &_renderers[i]);
			CHECKHR(hResult);
		}

		_currentRenderer = _renderers[0];
	}

CLEANUP:
	return hResult;
}

HRESULT RenderingManager::EnsureHWND()
{
	HRESULT hResult = S_OK;

	if (!_hwnd)
	{
		WNDCLASS windowClass;
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = DefWindowProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = NULL;
		windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		windowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
		windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		windowClass.lpszMenuName = NULL;
		windowClass.lpszClassName = AppName;

		if (!RegisterClass(&windowClass))
		{
			hResult = E_FAIL;
			CHECKHR(hResult);
		}

		_hwnd = CreateWindow(AppName, TEXT("D3DImageSample"), WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
	}

CLEANUP:
	return hResult;
}

HRESULT RenderingManager::EnsureD3DObjects()
{
	HRESULT hResult = S_OK;

	HMODULE hD3D = NULL;
	if (!_d3d)
	{
		hD3D = LoadLibrary(TEXT("d3d9.dll"));
		DIRECT3DCREATE9EXFUNCTION pfnCreate9Ex = (DIRECT3DCREATE9EXFUNCTION)GetProcAddress(hD3D, "Direct3DCreate9Ex");
		if (pfnCreate9Ex)
		{
			hResult = (*pfnCreate9Ex)(D3D_SDK_VERSION, &_d3dEx);
			CHECKHR(hResult);

			hResult = _d3dEx->QueryInterface(__uuidof(IDirect3D9), reinterpret_cast<void**>(&_d3d));
			CHECKHR(hResult);
		}
		else
		{
			_d3d = Direct3DCreate9(D3D_SDK_VERSION);
			if (!_d3d) {
				hResult = E_FAIL;
				CHECKHR(hResult);
			}
		}

		_adaptersCount = _d3d->GetAdapterCount();
	}

CLEANUP:
	if (hD3D)
	{
		FreeLibrary(hD3D);
	}

	return hResult;
}

void RenderingManager::CleanupInvalidDevices()
{
	HRESULT deviceState;
	for (UINT i = 0; i < _adaptersCount; i++)
	{
		deviceState = _renderers[i]->CheckDeviceState();
		if (FAILED(deviceState))
		{
			DestroyResources();
			break;
		}
	}
}

HRESULT RenderingManager::GetBackBufferNoRef(IDirect3DSurface9** surface)
{
	HRESULT hResult = S_OK;

	*surface = NULL;

	CleanupInvalidDevices();

	hResult = EnsureD3DObjects();
	CHECKHR(hResult);

	hResult = EnsureRenderers();
	CHECKHR(hResult);

	if (_surfaceSettingsChanged)
	{
		hResult = TestSurfaceSettings();
		CHECKHR(hResult);

		for (UINT i = 0; i < _adaptersCount; i++)
		{
			hResult = _renderers[i]->CreateSurface(_width, _height, _useAlpha, _desiredSamplesCount);
			CHECKHR(hResult);
		}

		_surfaceSettingsChanged = false;
	}

	if (_currentRenderer)
	{
		*surface = _currentRenderer->GetSurfaceNoRef();
	}

CLEANUP:
	// If we failed because of a bad device, ignore the failure for now and 
	// we'll clean up and try again next time.
	// https://docs.microsoft.com/en-us/dotnet/framework/wpf/advanced/walkthrough-creating-direct3d9-content-for-hosting-in-wpf
	if (hResult == D3DERR_DEVICELOST)
	{
		hResult = S_OK;
	}

	return hResult;
}

HRESULT RenderingManager::TestSurfaceSettings()
{
	HRESULT hResult = S_OK;

	D3DFORMAT format = _useAlpha ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8;
	for (UINT i = 0; i < _adaptersCount; i++)
	{
		hResult = _d3d->CheckDeviceType(i, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, format, TRUE);
		CHECKHR(hResult);

		hResult = _d3d->CheckDeviceFormat(i, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, format);
		CHECKHR(hResult);

		if (_d3dEx && _desiredSamplesCount > 1)
		{
			assert(_desiredSamplesCount <= 16);

			hResult = _d3d->CheckDeviceMultiSampleType(i, D3DDEVTYPE_HAL, format, TRUE, static_cast<D3DMULTISAMPLE_TYPE>(_desiredSamplesCount), NULL);
			if (FAILED(hResult))
			{
				_desiredSamplesCount = 0;
				hResult = S_OK;
			}
		}
		else
		{
			_desiredSamplesCount = 0;
		}
	}

CLEANUP:
	return hResult;
}

void RenderingManager::DestroyResources()
{
	SafeRelease(&_d3d);
	SafeRelease(&_d3dEx);

	for (UINT i = 0; i < _adaptersCount; i++)
	{
		delete _renderers[i];
	}
	delete[] _renderers;
	_renderers = nullptr;

	_currentRenderer = nullptr;
	_adaptersCount = 0;

	_surfaceSettingsChanged = true;
}

void RenderingManager::SetSize(UINT width, UINT height)
{
	if (_height != height || _width != width)
	{
		_width = width;
		_height = height;
		_surfaceSettingsChanged = true;
	}
}

int RenderingManager::GetWidth()
{
	return _width;
}

int RenderingManager::GetHeight()
{
	return _height;
}

void RenderingManager::SetAlpha(bool useAlpha)
{
	if (_useAlpha != useAlpha)
	{
		_useAlpha = useAlpha;
		_surfaceSettingsChanged = true;
	}
}

bool RenderingManager::GetAlpha()
{
	return _useAlpha;
}

void RenderingManager::SetDesiredSampledCount(UINT numDesiredSamples)
{
	if (_desiredSamplesCount != numDesiredSamples)
	{
		_desiredSamplesCount = numDesiredSamples;
		_surfaceSettingsChanged = true;
	}
}

int RenderingManager::GetDesiredSampledCount()
{
	return _desiredSamplesCount;
}

void RenderingManager::SetAdapter(POINT screenSpacePoint)
{
	CleanupInvalidDevices();

	if (_d3d && _renderers)
	{
		HMONITOR hMonitor = MonitorFromPoint(screenSpacePoint, MONITOR_DEFAULTTONULL);

		for (UINT i = 0; i < _adaptersCount; i++)
		{
			if (hMonitor == _d3d->GetAdapterMonitor(i))
			{
				_currentRenderer = _renderers[i];
				break;
			}
		}
	}
}

HRESULT RenderingManager::Render()
{
	return _currentRenderer ? _currentRenderer->Render() : S_OK;
}