#pragma once
#include "DXGISharedTypes.h"
#include "DXGISurfaceQueue.h"
#include "ISurfaceProducer.h"
#include "ISurfaceQueueDevice.h"

namespace ScreenCapture
{
    class DXGISurfaceQueue;

    class DXGISurfaceProducer : public ISurfaceProducer
    {
    public:
        DXGISurfaceProducer(BOOL isMultithreaded);
        ~DXGISurfaceProducer();

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** ppInterface);
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();

        HRESULT Enqueue(_In_ IUnknown* surface, _In_ void* buffer, _In_ UINT bufferSize, _In_ DWORD flags);
        HRESULT Flush(_In_ DWORD flags, _Out_ UINT* surfaces);
        HRESULT Initialize(IUnknown* device, UINT surfacesCount, SHARED_SURFACE_QUEUE_DESC* queueDescription);
        void SetQueue(DXGISurfaceQueue* queue);

        ISurfaceQueueDevice* GetDevice();

    private:
        ULONG _refCount;

        DXGISurfaceQueue* _queue;
        CRITICAL_SECTION _lock;
        BOOL _isMultithreaded;
        ISurfaceQueueDevice* _device;
        IUnknown** _stagingResources;
        UINT _stagingResourceWidth;
        UINT _stagingResourceHeight;
        UINT _currentResource;
        UINT _stagingResourcesCount;
    };
}