#pragma once
#include "ISurfaceQueue.h"
#include "DXGISurfaceConsumer.h"
#include "DXGISurfaceProducer.h"
#include "SharedDXGISurface.h"

namespace ScreenCapture
{
    class DXGISurfaceConsumer;
    class DXGISurfaceProducer;

    class DXGISurfaceQueue : public ISurfaceQueue
    {
    public:
        DXGISurfaceQueue();

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** ppInterface);
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();

        HRESULT Initialize(_In_ SHARED_SURFACE_QUEUE_DESC* description, _In_ IUnknown* device, _In_ DXGISurfaceQueue* surfaceQueue);

        HRESULT OpenProducer(_In_ IUnknown* device, _Out_ ISurfaceProducer** pProducer);
        HRESULT OpenConsumer(_In_ IUnknown* device, _Out_ ISurfaceConsumer** pConsumer);
        HRESULT Clone(_In_ SHARED_SURFACE_QUEUE_CLONE_DESC* description, _Out_ ISurfaceQueue** pQueue);
        void RemoveConsumer();
        void RemoveProducer();

        HRESULT Enqueue(_In_ IUnknown* surface, _In_ void* buffer, _In_ UINT bufferSize, _In_ DWORD flags, _In_ IUnknown* stagingResource, _In_ UINT width, _In_ UINT height);
        HRESULT Dequeue(IUnknown** surface, void* buffer, UINT* bufferSize, DWORD timeout);
        HRESULT Flush(_In_ DWORD flags, _Out_ UINT* surfacesCount);

    private:
        struct SharedDXGISurfaceQueueEntry
        {
            SharedDXGISurfaceObject* object = nullptr;
            BYTE* metadata = nullptr;
            UINT metadataSize = 0;
            IUnknown* stagingResource = nullptr;
        };

        struct SharedDXGISurfaceOpenedMapping
        {
            SharedDXGISurfaceObject* object = nullptr;
            IUnknown* surface = nullptr;
        };

    private:
        UINT AddQueueToNetwork();

        HRESULT CreateSurfaces();
        void CopySurfaceReferences(DXGISurfaceQueue* surfaceQueue);
        HRESULT AllocateMetaDataBuffers();

        void Enqueue(SharedDXGISurfaceQueueEntry& queueEntry);
        void Dequeue(SharedDXGISurfaceQueueEntry& queueEntry);
        SharedDXGISurfaceObject* GetSurfaceObjectFromHandle(HANDLE handle);
        void Front(SharedDXGISurfaceQueueEntry& queueEntry);
        IUnknown* GetOpenedSurface(const SharedDXGISurfaceObject* object) const;

    private:
        ULONG _refCount;

        SHARED_SURFACE_QUEUE_DESC _description;
        DXGISurfaceQueue* _rootQueue;
        bool _isMultithreaded;
        volatile UINT _queuesCountInNetwork;
        CRITICAL_SECTION _queueLock;
        SRWLOCK _srwlock;
        SharedDXGISurfaceQueueEntry* _surfaceQueue;
        SharedDXGISurfaceOpenedMapping* _consumerSurfaces;
        SharedDXGISurfaceObject** _createdSurfaces;
        ISurfaceQueueDevice* _creator;
        UINT _queueSize;
        DXGISurfaceProducer* _producer;
        DXGISurfaceConsumer* _consumer;
        UINT _enqueuedHead;
        UINT _enqueuedSurfacesCount;
        UINT _queueHead;

        union
        {
            HANDLE _semaphore;
            UINT _flushedSurfacesCount;
        };
    };

    HRESULT CreateSurfaceQueue(_In_ SHARED_SURFACE_QUEUE_DESC* description, _In_ IUnknown* device, _Out_ ISurfaceQueue** pQueue);
    HRESULT CreateDeviceWrapper(_In_ IUnknown* device, _Out_ ISurfaceQueueDevice** surfaceQueueDevice);
}