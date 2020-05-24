#pragma once
#include "DXGISurfaceQueue.h"
#include "ISurfaceConsumer.h"
#include "ISurfaceQueueDevice.h"

namespace ScreenCapture
{
    class DXGISurfaceQueue;

    class DXGISurfaceConsumer : public ISurfaceConsumer
    {
    public:
        DXGISurfaceConsumer(bool isMultithreaded);
        ~DXGISurfaceConsumer();

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** ppInterface);
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();

        HRESULT Initialize(IUnknown* device);
        HRESULT Dequeue(_In_ REFIID id, _Out_ IUnknown** ppSurface, _In_ _Out_ void* pBuffer, _In_ _Out_ UINT* pBufferSize, _In_ DWORD timeout);
        void SetQueue(DXGISurfaceQueue* queue);
        ISurfaceQueueDevice* GetDevice();

    private:
        LONG _refCount;

        DXGISurfaceQueue* _queue;
        bool _isMultithreaded;
        CRITICAL_SECTION _lock;
        ISurfaceQueueDevice* _device;
    };
}