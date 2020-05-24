#pragma once
#include "ISurfaceQueueDevice.h"

namespace ScreenCapture
{
    class D3D11SurfaceQueueDevice : public ISurfaceQueueDevice
    {
    public:
        D3D11SurfaceQueueDevice(ID3D11Device* device);
        ~D3D11SurfaceQueueDevice();

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** ppInterface);
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();

        HRESULT CreateSharedSurface(UINT width, UINT height, DXGI_FORMAT format, IUnknown** surface, HANDLE* handle);
        bool ValidateREFIID(REFIID id);
        HRESULT OpenSurface(HANDLE handle, void**, UINT width, UINT height, DXGI_FORMAT format);
        HRESULT GetSharedHandle(IUnknown* surface, HANDLE* sharedHandle);
        HRESULT CreateCopyResource(DXGI_FORMAT format, UINT width, UINT height, IUnknown** resource);
        HRESULT CopySurface(IUnknown* destinationSurface, IUnknown* sourceSurface, UINT width, UINT height);
        HRESULT LockSurface(IUnknown* surface, DWORD flags);
        HRESULT UnlockSurface(IUnknown* surface);

    private:
        ID3D11Device* _device;
        UINT _refCount;
    };
}