#include "pch.h"
#include "DXGISharedTypes.h"
#include "D3D11SurfaceQueueDevice.h"

using namespace ScreenCapture;

D3D11SurfaceQueueDevice::D3D11SurfaceQueueDevice(ID3D11Device* device)
    : _device(device)
{
    assert(_device);
    if (_device)
    {
        _device->AddRef();
    }
}

D3D11SurfaceQueueDevice::~D3D11SurfaceQueueDevice()
{
    SafeRelease(&_device);
}

HRESULT STDMETHODCALLTYPE D3D11SurfaceQueueDevice::QueryInterface(REFIID id, void** ppInterface)
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

ULONG STDMETHODCALLTYPE D3D11SurfaceQueueDevice::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG STDMETHODCALLTYPE D3D11SurfaceQueueDevice::Release()
{
    ULONG refCount = InterlockedDecrement(&_refCount);
    if (refCount == 0)
    {
        delete this;
    }
    return refCount;
}

HRESULT D3D11SurfaceQueueDevice::CreateSharedSurface(UINT width, UINT height, DXGI_FORMAT format, IUnknown** surface, HANDLE* handle)
{
    assert(_device);
    assert(surface);
    assert(handle);

    if (!_device || !surface || !handle)
    {
        return E_FAIL;
    }

    HRESULT hResult = S_OK;

    ID3D11Texture2D** texture = (ID3D11Texture2D**)surface;

    D3D11_TEXTURE2D_DESC textureDescription;
    textureDescription.Width = width;
    textureDescription.Height = height;
    textureDescription.MipLevels = 1;
    textureDescription.ArraySize = 1;
    textureDescription.Format = format;
    textureDescription.SampleDesc.Count = 1;
    textureDescription.SampleDesc.Quality = 0;
    textureDescription.Usage = D3D11_USAGE_DEFAULT;
    textureDescription.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDescription.CPUAccessFlags = 0;
    textureDescription.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

    hResult = _device->CreateTexture2D(&textureDescription, NULL, texture);
    if (SUCCEEDED(hResult))
    {
        if (FAILED(GetSharedHandle(*surface, handle)))
        {
            SafeRelease(texture);
        }
    }
    return hResult;
}

bool D3D11SurfaceQueueDevice::ValidateREFIID(REFIID id)
{
    return id == __uuidof(ID3D11Texture2D) || id == __uuidof(IDXGISurface);
}

HRESULT D3D11SurfaceQueueDevice::OpenSurface(HANDLE handle, void** ppSurface, UINT width, UINT height, DXGI_FORMAT format)
{
    return _device->OpenSharedResource(handle, __uuidof(ID3D11Texture2D), ppSurface);
}

HRESULT D3D11SurfaceQueueDevice::GetSharedHandle(IUnknown* surface, HANDLE* sharedHandle)
{
    assert(surface);
    assert(sharedHandle);
    if (!surface || !sharedHandle)
    {
        return E_INVALIDARG;
    }

    IDXGIResource* pSurface;

    HRESULT hResult = surface->QueryInterface(IID_PPV_ARGS(&pSurface));
    CHECKHR(hResult);

    hResult = pSurface->GetSharedHandle(sharedHandle);
    pSurface->Release();

CLEANUP:
    return hResult;
}

HRESULT D3D11SurfaceQueueDevice::CreateCopyResource(DXGI_FORMAT format, UINT width, UINT height, IUnknown** resource)
{
    assert(resource);
    assert(_device);
    if (!_device || !resource)
    {
        return E_INVALIDARG;
    }

    D3D11_TEXTURE2D_DESC textureDescription;
    textureDescription.Width = width;
    textureDescription.Height = height;
    textureDescription.MipLevels = 1;
    textureDescription.ArraySize = 1;
    textureDescription.Format = format;
    textureDescription.SampleDesc.Count = 1;
    textureDescription.SampleDesc.Quality = 0;
    textureDescription.Usage = D3D11_USAGE_STAGING;
    textureDescription.BindFlags = 0;
    textureDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    textureDescription.MiscFlags = 0;

    return _device->CreateTexture2D(&textureDescription, NULL, reinterpret_cast<ID3D11Texture2D**>(resource));
}

HRESULT D3D11SurfaceQueueDevice::CopySurface(IUnknown* destinationSurface, IUnknown* sourceSurface, UINT width, UINT height)
{
    D3D11_BOX unitBox = { 0, 0, width, height, 1 };
    ID3D11DeviceContext* context = nullptr;
    ID3D11Resource* sourceResource = nullptr;
    ID3D11Resource* destinationResource = nullptr;

    _device->GetImmediateContext(&context);
    assert(context);

    HRESULT hResult = destinationSurface->QueryInterface(IID_PPV_ARGS(&destinationResource));
    CHECKHR(hResult);

    hResult = sourceSurface->QueryInterface(IID_PPV_ARGS(&sourceResource));
    CHECKHR(hResult);

    context->CopySubresourceRegion(destinationResource, 0, 0, 0, 0, sourceResource, 0, &unitBox);

CLEANUP:
    SafeRelease(&sourceResource);
    SafeRelease(&destinationResource);
    SafeRelease(&context);
    return hResult;
}

HRESULT D3D11SurfaceQueueDevice::LockSurface(IUnknown* surface, DWORD flags)
{
    assert(surface);
    if (!surface)
    {
        return E_INVALIDARG;
    }

    HRESULT hResult = S_OK;
    D3D11_MAPPED_SUBRESOURCE region;
    ID3D11Resource* resource = nullptr;
    ID3D11DeviceContext* context = nullptr;
    DWORD d3d11flags = 0;

    _device->GetImmediateContext(&context);
    assert(context);

    if (flags & SURFACE_QUEUE_FLAG_DO_NOT_WAIT)
    {
        d3d11flags |= D3D11_MAP_FLAG_DO_NOT_WAIT;
    }

    hResult = surface->QueryInterface(IID_PPV_ARGS(&resource));
    CHECKHR(hResult);

    hResult = context->Map(resource, 0, D3D11_MAP_READ, d3d11flags, &region);
    CHECKHR(hResult);

CLEANUP:
    SafeRelease(&resource);
    SafeRelease(&context);
    return hResult;

}

HRESULT D3D11SurfaceQueueDevice::UnlockSurface(IUnknown* surface)
{
    assert(surface);
    if (!surface)
    {
        return E_INVALIDARG;
    }

    ID3D11DeviceContext* context = nullptr;
    ID3D11Resource* resource = nullptr;

    _device->GetImmediateContext(&context);
    assert(context);

    HRESULT hResult = surface->QueryInterface(IID_PPV_ARGS(&resource));
    CHECKHR(hResult);

    context->Unmap(resource, 0);

CLEANUP:
    SafeRelease(&resource);
    SafeRelease(&context);
    return hResult;
}