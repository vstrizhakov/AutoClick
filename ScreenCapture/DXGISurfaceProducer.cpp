#include "pch.h"
#include "DXGISurfaceQueue.h"
#include "DXGISurfaceProducer.h"

using namespace ScreenCapture;

DXGISurfaceProducer::DXGISurfaceProducer(BOOL isMultithreaded) :
    _isMultithreaded(isMultithreaded),
    _queue(nullptr),
    _refCount(0),
    _device(nullptr),
    _stagingResources(nullptr),
    _stagingResourceWidth(0),
    _stagingResourceHeight(0),
    _currentResource(0),
    _stagingResourcesCount(0)
{
    if (_isMultithreaded)
    {
        InitializeCriticalSection(&_lock);
    }
}

DXGISurfaceProducer::~DXGISurfaceProducer()
{
    if (_queue)
    {
        _queue->RemoveProducer();
        _queue->Release();
    }

    if (_stagingResources)
    {
        for (UINT i = 0; i < _stagingResourcesCount; i++)
        {
            if (_stagingResources[i])
            {
                _stagingResources[i]->Release();
            }
        }
        delete[] _stagingResources;
    }

    if (_device)
    {
        delete _device;
    }

    if (_isMultithreaded)
    {
        DeleteCriticalSection(&_lock);
    }

}

HRESULT STDMETHODCALLTYPE DXGISurfaceProducer::QueryInterface(REFIID id, void** ppInterface)
{
    if (!ppInterface)
    {
        return E_INVALIDARG;
    }
    *ppInterface = nullptr;
    if (id == __uuidof(ISurfaceProducer))
    {
        *reinterpret_cast<ISurfaceProducer**>(ppInterface) = this;
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

ULONG STDMETHODCALLTYPE DXGISurfaceProducer::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG STDMETHODCALLTYPE DXGISurfaceProducer::Release()
{
    ULONG refCount = InterlockedDecrement(&_refCount);
    if (refCount == 0)
    {
        delete this;
    }
    return refCount;
}

HRESULT DXGISurfaceProducer::Initialize(IUnknown* device, UINT surfacesCount, SHARED_SURFACE_QUEUE_DESC* queueDescription)
{
    assert(device);
    assert(!_stagingResources && _stagingResourcesCount == 0);

    HRESULT hResult = S_OK;

    hResult = CreateDeviceWrapper(device, &_device);
    CHECKHR(hResult);

    _stagingResources = new IUnknown*[surfacesCount];
    ZeroMemory(_stagingResources, sizeof(IUnknown*) * surfacesCount);
    _stagingResourcesCount = surfacesCount;

    _stagingResourceWidth = min(queueDescription->Width, SHARED_SURFACE_COPY_SIZE);
    _stagingResourceHeight = min(queueDescription->Height, SHARED_SURFACE_COPY_SIZE);

    for (UINT i = 0; i < _stagingResourcesCount; i++)
    {
        hResult = _device->CreateCopyResource(queueDescription->Format, _stagingResourceWidth, _stagingResourceHeight, &(_stagingResources[i]));
        CHECKHR(hResult);
    }

CLEANUP:
    if (FAILED(hResult))
    {
        if (_stagingResources)
        {
            for (UINT i = 0; i < _stagingResourcesCount; i++)
            {
                SafeRelease(&(_stagingResources[i]));
            }
            delete[] _stagingResources;
            _stagingResources = nullptr;
            _stagingResourcesCount = 0;
        }
        SafeRelease(&_device);
    }
    return hResult;
}

HRESULT DXGISurfaceProducer::Enqueue(_In_ IUnknown* surface, _In_ void* buffer, _In_ UINT bufferSize, _In_ DWORD flags)
{
    assert(_queue);
    if (!_queue)
    {
        return E_INVALIDARG;
    }

    if (_isMultithreaded)
    {
        EnterCriticalSection(&_lock);
    }

    HRESULT hResult = S_OK;

    if (!_device || !surface || (flags && flags != SURFACE_QUEUE_FLAG_DO_NOT_WAIT))
    {
        hResult = E_INVALIDARG;
        goto CLEANUP;
    }

    hResult = _queue->Enqueue(surface, buffer, bufferSize, flags, _stagingResources[_currentResource], _stagingResourceWidth, _stagingResourceHeight);

    if (hResult == DXGI_ERROR_WAS_STILL_DRAWING)
    {
        _currentResource = (_currentResource + 1) % _stagingResourcesCount;
    }

CLEANUP:
    if (_isMultithreaded)
    {
        LeaveCriticalSection(&_lock);
    }
    return hResult;
}

HRESULT DXGISurfaceProducer::Flush(_In_ DWORD flags, _Out_ UINT* surfacesCount)
{
    assert(_queue);
    if (!_queue)
    {
        return E_INVALIDARG;
    }

    if (_isMultithreaded)
    {
        EnterCriticalSection(&_lock);
    }

    HRESULT hResult = S_OK;
    if (!_device || (flags && flags != SURFACE_QUEUE_FLAG_DO_NOT_WAIT))
    {
        hResult = E_INVALIDARG;
        goto CLEANUP;
    }

    _queue->Flush(flags, surfacesCount);

CLEANUP:
    if (_isMultithreaded)
    {
        LeaveCriticalSection(&_lock);
    }
    return hResult;
}

ISurfaceQueueDevice* DXGISurfaceProducer::GetDevice()
{
    return _device;
}

void DXGISurfaceProducer::SetQueue(DXGISurfaceQueue* queue)
{
    assert(!_queue && queue);
    if (_queue || !queue)
    {
        return;
    }

    _queue = queue;
    _queue->AddRef();
}