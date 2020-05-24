#pragma once
#include <DXGIFormat.h>
#include "ISurfaceQueue.h"
#include "ISurfaceQueueDevice.h"

namespace ScreenCapture
{
    struct SharedDXGISurfaceObject
    {
        HANDLE sharedHandle;
        SharedDXGISurfaceState State;

        UINT width;
        UINT height;
        DXGI_FORMAT format;

        union
        {
            ISurfaceQueue* queue;
            ISurfaceQueueDevice* device;
        };

        IUnknown* surface;

        SharedDXGISurfaceObject(UINT width, UINT height, DXGI_FORMAT format);
        ~SharedDXGISurfaceObject();
    };
}
