#include "pch.h"
#include "DXGISurfaceQueue.h"
#include "CaptureManager.h"
#include <cliext/vector>

using namespace cliext;
using namespace ScreenCapture;
using namespace System;
using namespace System::Windows;
using namespace System::Windows::Interop;

HRESULT CaptureManager::InitializeD3D11()
{
    D3D_FEATURE_LEVEL* featureLevels = new D3D_FEATURE_LEVEL[1]
    {
        D3D_FEATURE_LEVEL_11_0,
    };

    IDXGIFactory1* factory = nullptr;
    HRESULT hResult = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    CHECKHR(hResult);

    IDXGIAdapter* adapter = nullptr;
    hResult = factory->EnumAdapters(0, &adapter);
    CHECKHR(hResult);

    {
        UINT createFlags = 0;
#ifdef _DEBUG
        createFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

        pin_ptr<ID3D11Device*> device = &_device;
        pin_ptr<ID3D11DeviceContext*> deviceContext = &_deviceContext;
        hResult = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags, featureLevels, 1, D3D11_SDK_VERSION, device, nullptr, deviceContext);
        CHECKHR(hResult);
    }

    IDXGIOutput* output = nullptr;
    hResult = adapter->EnumOutputs(0, &output);
    CHECKHR(hResult);

    DXGI_OUTPUT_DESC outputDescription;
    hResult = output->GetDesc(&outputDescription);
    CHECKHR(hResult);

    _screenSize = gcnew Size(outputDescription.DesktopCoordinates.right, outputDescription.DesktopCoordinates.bottom);

    D3D11_VIEWPORT viewport;
    viewport.Width = _screenSize->Width;
    viewport.Height = _screenSize->Height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    _deviceContext->RSSetViewports(1, &viewport);

    IDXGIOutput5* output5 = nullptr;
    hResult = output->QueryInterface(IID_PPV_ARGS(&output5));
    CHECKHR(hResult);

    {
        pin_ptr<IDXGIOutputDuplication*> outputDuplication = &_outputDuplication;
        hResult = output5->DuplicateOutput(_device, outputDuplication);
        CHECKHR(hResult);
    }

CLEANUP:
    return hResult;
}

CaptureManager::CaptureManager()
{
    _outputs = gcnew vector<RenderOutput^>();
    HRESULT hResult = InitializeD3D11();
    if (FAILED(hResult))
    {
        throw gcnew Exception("RenderingManager creation failed: " + hResult);
    }
}

CaptureManager::~CaptureManager()
{
}

RenderOutput^ CaptureManager::AddOutput(D3DImage^ d3dImage, Rect^ window)
{
    RenderOutput^ renderOutput = gcnew RenderOutput(_device, _deviceContext, d3dImage, _screenSize, window);
    _outputs->push_back(renderOutput);
    return renderOutput;
}

void CaptureManager::Render()
{
    HRESULT hResult = S_OK;

    IDXGIResource* frame = nullptr;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    hResult = _outputDuplication->AcquireNextFrame(INFINITE, &frameInfo, &frame);
    CHECKHR(hResult);

    ID3D11Resource* d3d11Frame = nullptr;
    hResult = frame->QueryInterface(IID_PPV_ARGS(&d3d11Frame));
    CHECKHR(hResult);
    
    for (vector<RenderOutput^>::iterator it = _outputs->begin(); it != _outputs->end(); it++)
    {
        RenderOutput^ output = *it;
        output->DrawFrame(d3d11Frame);
    }

    hResult = _outputDuplication->ReleaseFrame();
    CHECKHR(hResult);

CLEANUP:
    return;
}