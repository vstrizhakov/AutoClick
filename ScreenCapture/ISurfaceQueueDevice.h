#pragma once
#include <DXGIFormat.h>

namespace ScreenCapture
{
    class __declspec(uuid("{23AB264D-7641-4889-94F7-E55B587D8A26}")) ISurfaceQueueDevice : public IUnknown
    {
    public:
        virtual HRESULT CreateSharedSurface(UINT width, UINT height, DXGI_FORMAT format, IUnknown** surface, HANDLE* handle) = 0;
        virtual bool ValidateREFIID(REFIID id) = 0;
        virtual HRESULT OpenSurface(HANDLE handle, void**, UINT width, UINT height, DXGI_FORMAT format) = 0;
        virtual HRESULT GetSharedHandle(IUnknown* surface, HANDLE* sharedHandle) = 0;
        virtual HRESULT CreateCopyResource(DXGI_FORMAT format, UINT width, UINT height, IUnknown** resource) = 0;
        virtual HRESULT CopySurface(IUnknown* destinationSurface, IUnknown* sourceSurface, UINT width, UINT height) = 0;
        virtual HRESULT LockSurface(IUnknown* surface, DWORD flags) = 0;
        virtual HRESULT UnlockSurface(IUnknown* surface) = 0;
        virtual ~ISurfaceQueueDevice() {};
    };
}

