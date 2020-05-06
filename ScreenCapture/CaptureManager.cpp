#include "pch.h"
#include "RenderingManager.h"
#include "CaptureManager.h"

using namespace ScreenCapture;
using namespace System;
using namespace System::Windows;
using namespace System::Windows::Interop;

CaptureManager::CaptureManager(int width, int height, bool useAlpha, int desiredSamplesCount)
{
    RenderingManager* renderingManager = NULL;
    HRESULT hResult = RenderingManager::Create(&renderingManager);
    if (FAILED(hResult))
    {
        throw gcnew Exception("RenderingManager creation failed: " + hResult);
    }

    _renderingManager = renderingManager;
    Width = width;
    Height = height;
    UseAlpha = useAlpha;
    DesiredSamplesCount = desiredSamplesCount;
}

CaptureManager::~CaptureManager()
{
    delete _renderingManager;
    _renderingManager = nullptr;
}

void CaptureManager::Render(D3DImage^ d3dImage)
{
    d3dImage->Lock();

    IDirect3DSurface9* surface = NULL;
    _renderingManager->GetBackBufferNoRef(&surface);
    d3dImage->SetBackBuffer(D3DResourceType::IDirect3DSurface9, (IntPtr)surface);
    _renderingManager->Render();
    d3dImage->AddDirtyRect(Int32Rect(0, 0, d3dImage->PixelWidth, d3dImage->PixelHeight));
    d3dImage->Unlock();
}