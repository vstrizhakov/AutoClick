#include "pch.h"
#include "DXGISurfaceConsumer.h"

using namespace ScreenCapture;

DXGISurfaceConsumer::DXGISurfaceConsumer(bool isMultithreaded) :
    _isMultithreaded(isMultithreaded),
    _refCount(0),
    _queue(nullptr),
    _device(nullptr)
{
    if (_isMultithreaded)
    {
        InitializeCriticalSection(&_lock);
    }
}

DXGISurfaceConsumer::~DXGISurfaceConsumer()
{
    if (_queue)
    {
        _queue->RemoveConsumer();
        _queue->Release();
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

HRESULT STDMETHODCALLTYPE DXGISurfaceConsumer::QueryInterface(REFIID id, void** ppInterface)
{
    if (!ppInterface)
    {
        return E_INVALIDARG;
    }
    *ppInterface = nullptr;
    if (id == __uuidof(ISurfaceConsumer))
    {
        *reinterpret_cast<ISurfaceConsumer**>(ppInterface) = this;
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

ULONG STDMETHODCALLTYPE DXGISurfaceConsumer::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG STDMETHODCALLTYPE DXGISurfaceConsumer::Release()
{
    ULONG refCount = InterlockedDecrement(&_refCount);
    if (refCount == 0)
    {
        delete this;
    }
    return refCount;
}

HRESULT DXGISurfaceConsumer::Initialize(IUnknown* device)
{
    assert(device);
    assert(_device == nullptr);

    HRESULT hResult = CreateDeviceWrapper(device, &_device);
    if (FAILED(hResult))
    {
        if (_device)
        {
            delete _device;
            _device = nullptr;
        }
    }

    return hResult;
}

HRESULT DXGISurfaceConsumer::Dequeue(_In_ REFIID id, _Out_ IUnknown** ppSurface, _In_ _Out_ void* pBuffer, _In_ _Out_ UINT* pBufferSize, _In_ DWORD timeout)
{
    assert(_queue);
    if (!_queue)
    {
        return E_FAIL;
    }

    HRESULT hResult = S_OK;

    if (_isMultithreaded)
    {
        EnterCriticalSection(&_lock);
    }

    if (!_device->ValidateREFIID(id))
    {
        hResult = E_INVALIDARG;
        goto CLEANUP;
    }
    if (!ppSurface)
    {
        hResult = E_INVALIDARG;
        goto CLEANUP;
    }
    *ppSurface = nullptr;

    hResult = _queue->Dequeue(ppSurface, pBuffer, pBufferSize, timeout);
    CHECKHR(hResult);

CLEANUP:
    if (_isMultithreaded)
    {
        LeaveCriticalSection(&_lock);
    }
    return hResult;
}

void DXGISurfaceConsumer::SetQueue(DXGISurfaceQueue* queue)
{
    assert(!_queue && queue);
    if (_queue || !queue)
    {
        return;
    }

    _queue = queue;
    _queue->AddRef();
}

ISurfaceQueueDevice* DXGISurfaceConsumer::GetDevice()
{
    return _device;
}