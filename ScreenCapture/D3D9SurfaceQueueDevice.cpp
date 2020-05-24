#include "pch.h"
#include "DXGISharedTypes.h"
#include "D3D9SurfaceQueueDevice.h"

using namespace ScreenCapture;

static GUID SharedHandleGuid = { 0x91facf2d, 0xe464, 0x4495, 0x84, 0xa6, 0x37, 0xbe, 0xd3, 0x56, 0x8d, 0xa3 };

D3DFORMAT DXGIToCrossAPID3D9Format(DXGI_FORMAT format);

D3D9SurfaceQueueDevice::D3D9SurfaceQueueDevice(IDirect3DDevice9Ex* device)
    : _device(device)
{
    assert(_device);
    if (_device)
    {
        _device->AddRef();
    }
}

D3D9SurfaceQueueDevice::~D3D9SurfaceQueueDevice()
{
    SafeRelease(&_device);
}

HRESULT STDMETHODCALLTYPE D3D9SurfaceQueueDevice::QueryInterface(REFIID id, void** ppInterface)
{
    if (!ppInterface)
    {
        return E_INVALIDARG;
    }
    *ppInterface = nullptr;
    if (id == __uuidof(ISurfaceQueueDevice))
    {
        *reinterpret_cast<ISurfaceQueueDevice**>(ppInterface) = this;
        AddRef();
        return NOERROR;
    }
    else if (id == __uuidof(IUnknown))
    {
        *reinterpret_cast<IUnknown**>(ppInterface) = this;
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE D3D9SurfaceQueueDevice::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG STDMETHODCALLTYPE D3D9SurfaceQueueDevice::Release()
{
    ULONG refCount = InterlockedDecrement(&_refCount);
    if (refCount == 0)
    {
        delete this;
    }
    return refCount;
}

HRESULT D3D9SurfaceQueueDevice::CreateSharedSurface(UINT width, UINT height, DXGI_FORMAT format, IUnknown** surface, HANDLE* handle)
{
    assert(_device);
    if (!_device)
    {
        return E_INVALIDARG;
    }

    D3DFORMAT d3d9format;
    if ((d3d9format = DXGIToCrossAPID3D9Format(format)) == D3DFMT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    *handle = nullptr;
    HRESULT hResult = _device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, d3d9format, D3DPOOL_DEFAULT, reinterpret_cast<IDirect3DTexture9**>(surface), handle);
    return hResult;
}

bool D3D9SurfaceQueueDevice::ValidateREFIID(REFIID id)
{
    return id == __uuidof(IDirect3DTexture9);
}

HRESULT D3D9SurfaceQueueDevice::OpenSurface(HANDLE handle, void** ppUnknown, UINT width, UINT height, DXGI_FORMAT format)
{
    D3DFORMAT d3d9format;
    if ((d3d9format = DXGIToCrossAPID3D9Format(format)) == D3DFMT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    IDirect3DTexture9** ppTexture = reinterpret_cast<IDirect3DTexture9**>(ppUnknown);

    HRESULT hResult = _device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, d3d9format, D3DPOOL_DEFAULT, ppTexture, &handle);
    if (SUCCEEDED(hResult))
    {
        hResult = (*ppTexture)->SetPrivateData(SharedHandleGuid, &handle, sizeof(HANDLE), 0);
        if (FAILED(hResult))
        {
            SafeRelease(ppTexture);
        }
    }
    return hResult;
}

HRESULT D3D9SurfaceQueueDevice::GetSharedHandle(IUnknown* surface, HANDLE* sharedHandle)
{
    assert(surface);
    assert(sharedHandle);
    if (!surface || !sharedHandle)
    {
        return E_INVALIDARG;
    }

    *sharedHandle = nullptr;
    IDirect3DTexture9* pTexture = nullptr;

    HRESULT hResult = surface->QueryInterface(IID_PPV_ARGS(&pTexture));
    CHECKHR(hResult);

    DWORD size = sizeof(HANDLE);
    hResult = pTexture->GetPrivateData(SharedHandleGuid, sharedHandle, &size);
    pTexture->Release();

CLEANUP:
    return hResult;
}

HRESULT D3D9SurfaceQueueDevice::CreateCopyResource(DXGI_FORMAT format, UINT width, UINT height, IUnknown** resource)
{
    D3DFORMAT d3d9format;
    if ((d3d9format = DXGIToCrossAPID3D9Format(format)) == D3DFMT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    return _device->CreateRenderTarget(width, height, d3d9format, D3DMULTISAMPLE_NONE, 0, true, reinterpret_cast<IDirect3DSurface9**>(resource), nullptr);
}

HRESULT D3D9SurfaceQueueDevice::CopySurface(IUnknown* destination, IUnknown* source, UINT width, UINT height)
{
    assert(destination);
    assert(source);
    assert(_device);

    if (!destination || !source || !_device)
    {
        return E_INVALIDARG;
    }

    IDirect3DSurface9* sourceSurface = nullptr;
    IDirect3DSurface9* destinationSurface = nullptr;
    IDirect3DTexture9* sourceTexture = nullptr;
    RECT rect = { (long)0, (long)0, (long)width, (long)height };

    HRESULT hResult = source->QueryInterface(IID_PPV_ARGS(&sourceTexture));
    CHECKHR(hResult);

    hResult = sourceTexture->GetSurfaceLevel(0, &sourceSurface);
    CHECKHR(hResult);

    hResult = destination->QueryInterface(IID_PPV_ARGS(&destinationSurface));
    CHECKHR(hResult);

    hResult = _device->StretchRect(sourceSurface, &rect, destinationSurface, &rect, D3DTEXF_NONE);
    CHECKHR(hResult);

CLEANUP:
    SafeRelease(&sourceTexture);
    SafeRelease(&sourceSurface);
    SafeRelease(&destinationSurface);

    return hResult;
}

HRESULT D3D9SurfaceQueueDevice::LockSurface(IUnknown* unknown, DWORD flags)
{
    assert(unknown);
    if (!unknown)
    {
        return E_INVALIDARG;
    }

    IDirect3DSurface9* surface = nullptr;
    DWORD d3d9flags = D3DLOCK_READONLY;
    D3DLOCKED_RECT region;

    if (flags & SURFACE_QUEUE_FLAG_DO_NOT_WAIT)
    {
        d3d9flags |= D3DLOCK_DONOTWAIT;
    }

    HRESULT hResult = unknown->QueryInterface(IID_PPV_ARGS(&surface));
    CHECKHR(hResult);

    hResult = surface->LockRect(&region, nullptr, d3d9flags);
    CHECKHR(hResult);

CLEANUP:
    SafeRelease(&surface);

    if (hResult == D3DERR_WASSTILLDRAWING)
    {
        hResult = DXGI_ERROR_WAS_STILL_DRAWING;
    }

    return hResult;
}

HRESULT D3D9SurfaceQueueDevice::UnlockSurface(IUnknown* unknown)
{
    assert(unknown);
    if (!unknown)
    {
        return E_INVALIDARG;
    }

    IDirect3DSurface9* surface = nullptr;

    HRESULT hResult = unknown->QueryInterface(IID_PPV_ARGS(&surface));
    CHECKHR(hResult);

    hResult = surface->UnlockRect();
    CHECKHR(hResult);

CLEANUP:
    SafeRelease(&surface);

    return hResult;
}

D3DFORMAT DXGIToCrossAPID3D9Format(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return D3DFMT_A8R8G8B8;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return D3DFMT_A8R8G8B8;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return D3DFMT_X8R8G8B8;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return D3DFMT_A8B8G8R8;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return D3DFMT_A8B8G8R8;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        return D3DFMT_A2B10G10R10;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return D3DFMT_A16B16G16R16F;
    default:
        return D3DFMT_UNKNOWN;
    };
}