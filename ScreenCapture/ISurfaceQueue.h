#pragma once
#include "DXGISharedTypes.h"
#include "ISurfaceProducer.h"
#include "ISurfaceConsumer.h"

namespace ScreenCapture
{
    class __declspec(uuid("0E05F878-4132-48DB-B591-F0305986AA24")) ISurfaceQueue : public IUnknown
    {
    public:
        virtual HRESULT OpenProducer(_In_ IUnknown* device, ISurfaceProducer** pProducer) = 0;
        virtual HRESULT OpenConsumer(_In_ IUnknown* device, ISurfaceConsumer** pConsumer) = 0;
        virtual HRESULT Clone(_In_ SHARED_SURFACE_QUEUE_CLONE_DESC* description, ISurfaceQueue** pQueue) = 0;
    };
}

