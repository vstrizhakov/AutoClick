#include "pch.h"
#include "D3D9SurfaceQueueDevice.h"
#include "D3D11SurfaceQueueDevice.h"
#include "DXGISurfaceProducer.h"
#include "DXGISurfaceConsumer.h"
#include "DXGISurfaceQueue.h"

using namespace ScreenCapture;

#pragma region IUnknown interface implementation

HRESULT STDMETHODCALLTYPE DXGISurfaceQueue::QueryInterface(REFIID id, void** ppInterface)
{
    if (!ppInterface)
    {
        return E_INVALIDARG;
    }
    *ppInterface = nullptr;
    if (id == __uuidof(ISurfaceQueue))
    {
        *reinterpret_cast<ISurfaceQueue**>(ppInterface) = this;
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

ULONG STDMETHODCALLTYPE DXGISurfaceQueue::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG STDMETHODCALLTYPE DXGISurfaceQueue::Release()
{
    ULONG refCount = InterlockedDecrement(&_refCount);
    if (refCount == 0)
    {
        delete this;
    }
    return refCount;
}

#pragma endregion

#pragma region DXGISurfaceQueue class implementation

DXGISurfaceQueue::DXGISurfaceQueue() :
    _refCount(0),
    _rootQueue(nullptr),
    _surfaceQueue(nullptr),
    _consumerSurfaces(nullptr),
    _createdSurfaces(nullptr),
    _creator(nullptr),
    _producer(nullptr),
    _consumer(nullptr),
    _isMultithreaded(true),
    _queuesCountInNetwork(0),
    _queueSize(0),
    _enqueuedSurfacesCount(0),
    _enqueuedHead(0),
    _queueHead(0)
{

}

HRESULT DXGISurfaceQueue::Initialize(_In_ SHARED_SURFACE_QUEUE_DESC* description, _In_ IUnknown* device, _In_ DXGISurfaceQueue* rootQueue)
{
    HRESULT hResult = S_OK;

    assert(description);
    assert(rootQueue);
    if (!description || !rootQueue)
    {
        return E_FAIL;
    }

    _description = *description;
    _rootQueue = rootQueue;
    _isMultithreaded = !(_description.Flags & SURFACE_QUEUE_FLAG_SINGLE_THREADED);

    if (_isMultithreaded)
    {
        InitializeCriticalSection(&_queueLock);
    }

    assert(!_surfaceQueue);
    _surfaceQueue = new SharedDXGISurfaceQueueEntry[_description.NumSurfaces];
    ZeroMemory(_surfaceQueue, sizeof(SharedDXGISurfaceQueueEntry) * _description.NumSurfaces);

    assert(!_consumerSurfaces);
    _consumerSurfaces = new SharedDXGISurfaceOpenedMapping[_description.NumSurfaces];
    ZeroMemory(_consumerSurfaces, sizeof(SharedDXGISurfaceOpenedMapping) * _description.NumSurfaces);

    assert(!_createdSurfaces);
    _createdSurfaces = new SharedDXGISurfaceObject*[_description.NumSurfaces];
    ZeroMemory(_createdSurfaces, sizeof(SharedDXGISurfaceObject*) * _description.NumSurfaces);

    if (_rootQueue == this)
    {
        assert(device);
        hResult = CreateDeviceWrapper(device, &_creator);
        //assert(_creator == nullptr);
        CHECKHR(hResult);

        hResult = CreateSurfaces();
        CHECKHR(hResult);

        _queueSize = _description.NumSurfaces;
    }
    else
    {
        _rootQueue->AddRef();
        CopySurfaceReferences(_rootQueue);
        _queueSize = 0;
    }

    if (_description.MetaDataSize)
    {
        hResult = AllocateMetaDataBuffers();
        CHECKHR(hResult);
    }

    assert(_rootQueue);

    if (_isMultithreaded)
    {
        _semaphore = CreateSemaphore(nullptr, _rootQueue == this ? _description.NumSurfaces : 0, _description.NumSurfaces, nullptr);
        if (!_semaphore)
        {
            hResult = HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
        }
        InitializeSRWLock(&_srwlock);
    }
    else
    {
        _flushedSurfacesCount = _rootQueue == this ? _description.NumSurfaces : 0;
    }

CLEANUP:
    return hResult;
}

HRESULT DXGISurfaceQueue::AllocateMetaDataBuffers()
{
    if (_description.MetaDataSize)
    {
        for (UINT i = 0; i < _description.NumSurfaces; i++)
        {
            _surfaceQueue[i].metadata = new BYTE[_description.MetaDataSize];
        }
    }
    return S_OK;
}

UINT DXGISurfaceQueue::AddQueueToNetwork()
{
    if (_rootQueue == this)
    {
        return InterlockedIncrement(&_queuesCountInNetwork);
    }
    else
    {
        return _rootQueue->AddQueueToNetwork();
    }
}

HRESULT DXGISurfaceQueue::OpenProducer(_In_ IUnknown* device, ISurfaceProducer** pProducer)
{
    if (!device || !pProducer)
    {
        return E_INVALIDARG;
    }

    *pProducer = nullptr;

    HRESULT hResult = E_FAIL;

    if (_isMultithreaded)
    {
        AcquireSRWLockExclusive(&_srwlock);
    }

    if (_producer)
    {
        if (_isMultithreaded)
        {
            ReleaseSRWLockExclusive(&_srwlock);
        }
        return E_INVALIDARG;
    }

    _producer = new DXGISurfaceProducer(_isMultithreaded);

    hResult = _producer->Initialize(device, _description.NumSurfaces, &_description);
    CHECKHR(hResult);

    hResult = _producer->QueryInterface(IID_PPV_ARGS(pProducer));
    CHECKHR(hResult);

    _producer->SetQueue(this);

CLEANUP:
    if (FAILED(hResult))
    {
        *pProducer = nullptr;
        SafeRelease(&_producer);
    }

    if (_isMultithreaded)
    {
        ReleaseSRWLockExclusive(&_srwlock);
    }
    return hResult;
}

void DXGISurfaceQueue::RemoveProducer()
{
    if (_isMultithreaded)
    {
        AcquireSRWLockExclusive(&_srwlock);
    }

    assert(_producer);
    _producer = nullptr;

    if (_isMultithreaded)
    {
        ReleaseSRWLockExclusive(&_srwlock);
    }
}

HRESULT DXGISurfaceQueue::Enqueue(_In_ IUnknown* surface, _In_ void* buffer, _In_ UINT bufferSize, _In_ DWORD flags, _In_ IUnknown* stagingResource, _In_ UINT width, _In_ UINT height)
{
    assert(surface);
    if ((buffer && !bufferSize) || (!buffer && bufferSize) || bufferSize > _description.MetaDataSize)
    {
        return E_INVALIDARG;
    }

    HRESULT hResult = E_FAIL;

    if (_isMultithreaded)
    {
        AcquireSRWLockShared(&_srwlock);
    }

    assert(_producer);

    SharedDXGISurfaceQueueEntry queueEntry;
    HANDLE sharedHandle;
    SharedDXGISurfaceObject* surfaceObject;

    if (!_producer || !_consumer)
    {
        hResult = E_INVALIDARG;
        goto CLEANUP;
    }

    if (_queueSize == _description.NumSurfaces)
    {
        hResult = E_INVALIDARG;
        goto CLEANUP;
    }

    hResult = _producer->GetDevice()->GetSharedHandle(surface, &sharedHandle);
    CHECKHR(hResult);

    surfaceObject = GetSurfaceObjectFromHandle(sharedHandle);
    if (!surfaceObject)
    {
        hResult = E_INVALIDARG;
        goto CLEANUP;
    }

    if (surfaceObject->State != SHARED_DXGI_SURFACE_STATE_DEQUEUED)
    {
        hResult = E_INVALIDARG;
        goto CLEANUP;
    }

    queueEntry.object = surfaceObject;
    queueEntry.metadata = (BYTE*)buffer;
    queueEntry.metadataSize = bufferSize;
    queueEntry.stagingResource = NULL;

    hResult = _producer->GetDevice()->CopySurface(stagingResource, surface, width, height);
    CHECKHR(hResult);

    surfaceObject->State = SHARED_DXGI_SURFACE_STATE_ENQUEUED;
    surfaceObject->queue = this;

    if (flags & SURFACE_QUEUE_FLAG_DO_NOT_WAIT)
    {
        queueEntry.stagingResource = stagingResource;
        Enqueue(queueEntry);
        _enqueuedSurfacesCount++;

        hResult = DXGI_ERROR_WAS_STILL_DRAWING;
        goto CLEANUP;
    }
    else if (_enqueuedSurfacesCount)
    {
        hResult = Flush(0, NULL);
        assert(SUCCEEDED(hResult));
    }

    hResult = _producer->GetDevice()->LockSurface(stagingResource, flags);
    CHECKHR(hResult);

    hResult = _producer->GetDevice()->UnlockSurface(stagingResource);
    CHECKHR(hResult);

    assert(!queueEntry.stagingResource);
    surfaceObject->State = SHARED_DXGI_SURFACE_STATE_FLUSHED;

    _enqueuedHead = (_enqueuedHead + 1) % _enqueuedHead;
    Enqueue(queueEntry);

    if (_isMultithreaded)
    {
        ReleaseSemaphore(_semaphore, 1, nullptr);
    }
    else
    {
        _flushedSurfacesCount++;
    }

CLEANUP:
    if (_isMultithreaded)
    {
        ReleaseSRWLockShared(&_srwlock);
    }
    return hResult;
}

void DXGISurfaceQueue::Enqueue(SharedDXGISurfaceQueueEntry& queueEntry)
{
    if (_isMultithreaded)
    {
        EnterCriticalSection(&_queueLock);
    }

    UINT end = (_queueHead + _queueSize) % _description.NumSurfaces;
    _queueSize++;

    if (_isMultithreaded)
    {
        LeaveCriticalSection(&_queueLock);
    }

    _surfaceQueue[end].object = queueEntry.object;
    _surfaceQueue[end].metadataSize = queueEntry.metadataSize;
    _surfaceQueue[end].stagingResource = queueEntry.stagingResource;
    if (queueEntry.metadataSize)
    {
        memcpy(_surfaceQueue[end].metadata, queueEntry.metadata, sizeof(BYTE) * queueEntry.metadataSize);
    }
}

HRESULT DXGISurfaceQueue::Dequeue(IUnknown** surface, void* buffer, UINT* bufferSize, DWORD timeout)
{
    if (!buffer && bufferSize)
    {
        return E_INVALIDARG;
    }
    if (buffer)
    {
        if (!bufferSize || *bufferSize == 0)
        {
            return E_INVALIDARG;
        }
        if (*bufferSize > _description.MetaDataSize)
        {
            return E_INVALIDARG;
        }
    }

    if (_isMultithreaded)
    {
        AcquireSRWLockShared(&_srwlock);
    }

    SharedDXGISurfaceQueueEntry queueEntry;
    IUnknown* pSurface = nullptr;
    HRESULT hResult = E_FAIL;

    if (!_producer || !_consumer)
    {
        hResult = E_INVALIDARG;
        goto CLEANUP;
    }

    if (_isMultithreaded)
    {
        DWORD wait = WaitForSingleObject(_semaphore, timeout);
        switch (wait)
        {
        case WAIT_ABANDONED:
            hResult = E_FAIL;
            break;
        case WAIT_OBJECT_0:
            hResult = S_OK;
            break;
        case WAIT_TIMEOUT:
            hResult = HRESULT_FROM_WIN32(WAIT_TIMEOUT);
            break;
        case WAIT_FAILED:
            hResult = HRESULT_FROM_WIN32(GetLastError());
            break;
        default:
            hResult = E_FAIL;
            break;
        }
    }
    else
    {
        if (_flushedSurfacesCount == 0)
        {
            hResult = HRESULT_FROM_WIN32(WAIT_TIMEOUT);
        }
        else
        {
            _flushedSurfacesCount--;
            hResult = S_OK;
        }
    }

    CHECKHR(hResult);

    Front(queueEntry);

    assert(queueEntry.object->State == SHARED_DXGI_SURFACE_STATE_FLUSHED);
    assert(queueEntry.object->queue == this);

    queueEntry.object->State = SHARED_DXGI_SURFACE_STATE_DEQUEUED;
    queueEntry.object->device = _consumer->GetDevice();

    pSurface = GetOpenedSurface(queueEntry.object);
    assert(pSurface);
    pSurface->AddRef();
    *surface = pSurface;

    if (buffer && queueEntry.metadataSize)
    {
        memcpy(buffer, queueEntry.metadata, sizeof(BYTE) * queueEntry.metadataSize);
    }

    if (bufferSize)
    {
        *bufferSize = queueEntry.metadataSize;
    }

    Dequeue(queueEntry);

CLEANUP:
    if (_isMultithreaded)
    {
        ReleaseSRWLockShared(&_srwlock);
    }

    return hResult;
}

void DXGISurfaceQueue::Dequeue(SharedDXGISurfaceQueueEntry& queueEntry)
{
    if (_isMultithreaded)
    {
        EnterCriticalSection(&_queueLock);
    }

    queueEntry = _surfaceQueue[_queueHead];
    _queueHead = (_queueHead + 1) % _description.NumSurfaces;
    _queueSize--;

    if (_isMultithreaded)
    {
        LeaveCriticalSection(&_queueLock);
    }
}

void DXGISurfaceQueue::Front(SharedDXGISurfaceQueueEntry& queueEntry)
{
    queueEntry = _surfaceQueue[_queueHead];
}

SharedDXGISurfaceObject* DXGISurfaceQueue::GetSurfaceObjectFromHandle(HANDLE handle)
{
    assert(handle);
    assert(_createdSurfaces);

    if (!handle || !_createdSurfaces)
    {
        return nullptr;
    }

    for (UINT i = 0; i < _description.NumSurfaces; i++)
    {
        if (_createdSurfaces[i]->sharedHandle == handle)
        {
            return _createdSurfaces[i];
        }
    }
    return nullptr;
}

HRESULT DXGISurfaceQueue::CreateSurfaces()
{
    HRESULT hResult = S_OK;

    assert(_rootQueue == this);
    if (_rootQueue == this)
    {
        for (UINT i = 0; i < _description.NumSurfaces; i++)
        {
            SharedDXGISurfaceObject* surfaceObject = new SharedDXGISurfaceObject(_description.Width, _description.Height, _description.Format);
            _createdSurfaces[i] = surfaceObject;

            hResult = _creator->CreateSharedSurface(_description.Width, _description.Height, _description.Format, &(surfaceObject->surface), &(surfaceObject->sharedHandle));
            CHECKHR(hResult);

            _surfaceQueue[i].object = _createdSurfaces[i];
            _surfaceQueue[i].object->State = SHARED_DXGI_SURFACE_STATE_FLUSHED;
            _surfaceQueue[i].object->queue = this;
        }
    }

CLEANUP:
    return hResult;
}

void DXGISurfaceQueue::CopySurfaceReferences(DXGISurfaceQueue* rootQueue)
{
    assert(_rootQueue != this);
    if (_rootQueue != this)
    {
        for (UINT i = 0; i < _description.NumSurfaces; i++)
        {
            _createdSurfaces[i] = _rootQueue->_createdSurfaces[i];
        }
    }
}

HRESULT DXGISurfaceQueue::OpenConsumer(IUnknown* device, ISurfaceConsumer** consumer)
{
    if (!device || !consumer)
    {
        return E_INVALIDARG;
    }

    *consumer = nullptr;

    HRESULT hResult = E_FAIL;
    if (_isMultithreaded)
    {
        AcquireSRWLockExclusive(&_srwlock);
    }

    if (_consumer)
    {
        if (_isMultithreaded)
        {
            ReleaseSRWLockExclusive(&_srwlock);
        }
        return E_INVALIDARG;
    }

    _consumer = new DXGISurfaceConsumer(_isMultithreaded);
    hResult = _consumer->Initialize(device);
    CHECKHR(hResult);

    for (UINT i = 0; i < _description.NumSurfaces; i++)
    {
        assert(_createdSurfaces[i]);
        assert(_consumerSurfaces);

        if (!_createdSurfaces[i] || !_consumerSurfaces)
        {
            return E_FAIL;
        }

        IUnknown* surface = nullptr;
        hResult = _consumer->GetDevice()->OpenSurface(_createdSurfaces[i]->sharedHandle, (void**)&surface, _description.Width, _description.Height, _description.Format);
        CHECKHR(hResult);

        assert(surface);
        if (surface)
        {
            _consumerSurfaces[i].object = _createdSurfaces[i];
            _consumerSurfaces[i].surface = surface;
        }
    }

    hResult = _consumer->QueryInterface(IID_PPV_ARGS(consumer));
    CHECKHR(hResult);

    _consumer->SetQueue(this);

CLEANUP:
    if (FAILED(hResult))
    {
        *consumer = nullptr;
        if (_consumer)
        {
            if (_consumer->GetDevice())
            {
                for (UINT i = 0; i < _description.NumSurfaces; i++)
                {
                    if (_consumerSurfaces[i].surface)
                    {
                        _consumerSurfaces[i].surface->Release();
                    }
                }
            }
            ZeroMemory(_consumerSurfaces, sizeof(SharedDXGISurfaceOpenedMapping) * _description.NumSurfaces);

            delete _consumer;
            _consumer = nullptr;
        }
    }

    if (_isMultithreaded)
    {
        ReleaseSRWLockExclusive(&_srwlock);
    }
    return hResult;
}

void DXGISurfaceQueue::RemoveConsumer()
{
    if (_isMultithreaded)
    {
        AcquireSRWLockExclusive(&_srwlock);
    }

    assert(_consumer && _consumer->GetDevice());
    for (UINT i = 0; i < _description.NumSurfaces; i++)
    {
        if (_consumerSurfaces[i].surface)
        {
            _consumerSurfaces[i].surface->Release();
        }
    }
    ZeroMemory(_consumerSurfaces, sizeof(SharedDXGISurfaceOpenedMapping) * _description.NumSurfaces);
    _consumer = nullptr;

    if (_isMultithreaded)
    {
        ReleaseSRWLockExclusive(&_srwlock);
    }
}

HRESULT DXGISurfaceQueue::Clone(_In_ SHARED_SURFACE_QUEUE_CLONE_DESC* description, _Out_ ISurfaceQueue** pQueue)
{
    if (_rootQueue != this)
    {
        return _rootQueue->Clone(description, pQueue);
    }

    if (!description || !pQueue || (description->Flags != 0 && description->Flags != SURFACE_QUEUE_FLAG_SINGLE_THREADED))
    {
        return E_INVALIDARG;
    }

    *pQueue = nullptr;
    HRESULT hResult = E_FAIL;

    if (_isMultithreaded)
    {
        AcquireSRWLockExclusive(&_srwlock);
    }

    SHARED_SURFACE_QUEUE_DESC createDescription = _description;
    createDescription.MetaDataSize = description->MetaDataSize;
    createDescription.Flags = description->Flags;

    DXGISurfaceQueue* clonedQueue = new DXGISurfaceQueue();
    hResult = clonedQueue->Initialize(&createDescription, NULL, this);
    CHECKHR(hResult);

    hResult = clonedQueue->QueryInterface(IID_PPV_ARGS(pQueue));
    CHECKHR(hResult);

CLEANUP:
    if (FAILED(hResult))
    {
        if (clonedQueue)
        {
            delete clonedQueue;
        }
        *pQueue = nullptr;
    }

    if (_isMultithreaded)
    {
        ReleaseSRWLockExclusive(&_srwlock);
    }

    return hResult;
}

HRESULT DXGISurfaceQueue::Flush(_In_ DWORD flags, _Out_ UINT* surfacesCount)
{
    if (_isMultithreaded)
    {
        AcquireSRWLockShared(&_srwlock);
    }

    HRESULT hResult = S_OK;
    UINT flushedSurfacesCount = 0;
    UINT index, i;

    UINT enqueuedSurfacesCount = _enqueuedSurfacesCount;
    if (!_producer || !_consumer)
    {
        hResult = E_INVALIDARG;
        goto CLEANUP;
    }

    for (index = _enqueuedHead, i = 0; i < enqueuedSurfacesCount; i++, index++)
    {
        index = index % _description.NumSurfaces;

        SharedDXGISurfaceQueueEntry& queueEntry = _surfaceQueue[index];

        assert(queueEntry.object->State == SHARED_DXGI_SURFACE_STATE_ENQUEUED);
        assert(queueEntry.object->queue == this);
        assert(queueEntry.stagingResource);

        IUnknown* stagingResource = queueEntry.stagingResource;
        hResult = _producer->GetDevice()->LockSurface(stagingResource, flags);
        CHECKHR(hResult);

        hResult = _producer->GetDevice()->UnlockSurface(stagingResource);
        assert(SUCCEEDED(hResult));

        queueEntry.object->State = SHARED_DXGI_SURFACE_STATE_FLUSHED;
        queueEntry.stagingResource = nullptr;

        flushedSurfacesCount++;

        _enqueuedSurfacesCount--;
        _enqueuedHead = (_enqueuedHead + 1) % _description.NumSurfaces;

        if (_isMultithreaded)
        {
            ReleaseSemaphore(_semaphore, 1, NULL);
        }
        else
        {
            _flushedSurfacesCount++;
        }
    }

CLEANUP:
    if (surfacesCount)
    {
        *surfacesCount = _enqueuedSurfacesCount;
    }

    if (_isMultithreaded)
    {
        ReleaseSRWLockShared(&_srwlock);
    }

    return hResult;
}

IUnknown* DXGISurfaceQueue::GetOpenedSurface(const SharedDXGISurfaceObject* object) const
{
    assert(object);
    assert(_consumerSurfaces);
    if (!object || !_consumerSurfaces)
    {
        return nullptr;
    }

    for (UINT i = 0; i < _description.NumSurfaces; i++)
    {
        if (_consumerSurfaces[i].object == object)
        {
            return _consumerSurfaces[i].surface;
        }
    }
    return nullptr;
}

#pragma endregion

HRESULT ScreenCapture::CreateSurfaceQueue(_In_ SHARED_SURFACE_QUEUE_DESC* description, _In_ IUnknown* device, _Out_ ISurfaceQueue** pSurfaceQueue)
{
    HRESULT hResult = E_FAIL;

    if (!pSurfaceQueue)
    {
        return E_INVALIDARG;
    }

    *pSurfaceQueue = nullptr;

    if (!description
        || description->NumSurfaces == 0
        || description->Width == 0
        || description->Height == 0
        || (description->Flags != 0 && description->Flags != SURFACE_QUEUE_FLAG_SINGLE_THREADED))
    {
        return E_INVALIDARG;
    }

   DXGISurfaceQueue* surfaceQueue = new DXGISurfaceQueue();
   hResult = surfaceQueue->Initialize(description, device, surfaceQueue);
   CHECKHR(hResult);

   hResult = surfaceQueue->QueryInterface(__uuidof(ISurfaceQueue), reinterpret_cast<void**>(pSurfaceQueue));

CLEANUP:
   if (FAILED(hResult))
   {
       if (surfaceQueue)
       {
           delete surfaceQueue;
       }
       *pSurfaceQueue = nullptr;
   }
   return hResult;
}

HRESULT ScreenCapture::CreateDeviceWrapper(_In_ IUnknown* device, _Out_ ISurfaceQueueDevice** surfaceQueueDevice)
{
    HRESULT hResult = S_OK;

    IDirect3DDevice9Ex* d3d9Device = nullptr;
    ID3D11Device* d3d11Device = nullptr;

    hResult = device->QueryInterface(IID_PPV_ARGS(&d3d9Device));
    if (SUCCEEDED(hResult))
    {
        d3d9Device->Release();
        *surfaceQueueDevice = new D3D9SurfaceQueueDevice(d3d9Device);
    }
    else
    {
        hResult = device->QueryInterface(IID_PPV_ARGS(&d3d11Device));
        if (SUCCEEDED(hResult))
        {
            d3d11Device->Release();
            *surfaceQueueDevice = new D3D11SurfaceQueueDevice(d3d11Device);
        }
    }

    return hResult;
}