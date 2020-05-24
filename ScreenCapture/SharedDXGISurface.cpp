#include "pch.h"
#include "SharedDXGISurface.h"

using namespace ScreenCapture;

SharedDXGISurfaceObject::SharedDXGISurfaceObject(UINT width, UINT height, DXGI_FORMAT format) :
    State(SHARED_DXGI_SURFACE_STATE_UNINITIALIZED),
    sharedHandle(nullptr),
    queue(nullptr),
    width(width),
    height(height),
    format(format),
    surface(nullptr)
{

}

SharedDXGISurfaceObject::~SharedDXGISurfaceObject()
{
    SafeRelease(&surface);
}