#include "pch.h"
#include "DXGISurfaceQueue.h"
#include "CaptureManager.h"
#include <time.h>
#include <stdlib.h>

using namespace ScreenCapture;
using namespace System;
using namespace System::Windows;
using namespace System::Windows::Interop;

struct XMFLOAT3
{
    float x;
    float y;
    float z;
};

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT3 texcoord;

    Vertex(float x, float y, float z, float tx, float ty)
    {
        position.x = x;
        position.y = y;
        position.z = z;
        texcoord.x = tx;
        texcoord.y = ty;
    }
};

const char* shader =
"\
SamplerState samplerState : register(s0);\
Texture2D currentTexture : register(t0);\
struct VS_INPUT\
{\
    float4 pos : POSITION;\
    float3 tex : TEXCOORD;\
};\
struct PS_INPUT\
{\
    float4 pos : SV_POSITION;\
    float3 tex : TEXCOORD0;\
};\
PS_INPUT VS(VS_INPUT input)\
{\
    PS_INPUT output = (PS_INPUT)0;\
    output.pos = input.pos;\
    output.tex = input.tex;\
    return output;\
}\
float4 PS(PS_INPUT input) : SV_Target\
{\
    float4 finalColor = currentTexture.Sample(samplerState, input.tex);\
    float grey = finalColor.r * 0.3 + finalColor.g * 0.59 + finalColor.b * 0.11;\
    return float4(grey, grey, grey, 1);\
}\
";

HRESULT CompileShader(const char* source, LPCSTR entryPoint, LPCSTR profile, ID3D10Blob** blob)
{
    if (!source || !entryPoint || !profile || !blob)
    {
        return E_INVALIDARG;
    }

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG;
#endif
    ID3D10Blob* resultBlob = NULL;
    ID3D10Blob* errorBlob = NULL;

    HRESULT result = D3DCompile(source, strlen(source), NULL, NULL, NULL, entryPoint, profile, flags, 0, &resultBlob, &errorBlob);

    if (FAILED(result))
    {
        if (errorBlob != NULL)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        SafeRelease(&errorBlob);
        SafeRelease(&resultBlob);
        return result;
    }
    *blob = resultBlob;
    return result;
}

HRESULT CaptureManager::InitializeD3D11(UINT width, UINT height)
{
    srand(time(NULL));

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
        //createFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

        pin_ptr<ID3D11Device*> device = &_device;
        pin_ptr<ID3D11DeviceContext*> deviceContext = &_deviceContext;
        hResult = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags, featureLevels, 1, D3D11_SDK_VERSION, device, nullptr, deviceContext);
        CHECKHR(hResult);
    }

    D3D11_VIEWPORT viewport;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    _deviceContext->RSSetViewports(1, &viewport);

    IDXGIOutput* output = nullptr;
    hResult = adapter->EnumOutputs(0, &output);
    CHECKHR(hResult);

    IDXGIOutput5* output5 = nullptr;
    hResult = output->QueryInterface(IID_PPV_ARGS(&output5));
    CHECKHR(hResult);

    {
        pin_ptr<IDXGIOutputDuplication*> outputDuplication = &_outputDuplication;
        hResult = output5->DuplicateOutput(_device, outputDuplication);
        CHECKHR(hResult);
    }

    Vertex vertices[] =
    {
        Vertex(-1.f, -1.f, 1.f, 0.f, 1.f),
        Vertex(-1.f,  1.f, 1.f, 0.f, 0.f),
        Vertex( 1.f,  1.f, 1.f, 1.f, 0.f),
        Vertex( 1.f,  1.f, 1.f, 1.f, 0.f),
        Vertex( 1.f, -1.f, 1.f, 1.f, 1.f),
        Vertex(-1.f, -1.f, 1.f, 0.f, 1.f),
    };

    D3D11_BUFFER_DESC vertexBufferDescription;
    ZeroMemory(&vertexBufferDescription, sizeof(D3D11_BUFFER_DESC));
    vertexBufferDescription.ByteWidth = sizeof(vertices);
    vertexBufferDescription.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDescription.CPUAccessFlags = 0;
    vertexBufferDescription.MiscFlags = 0;
    vertexBufferDescription.StructureByteStride = sizeof(Vertex);

    D3D11_SUBRESOURCE_DATA vertexBufferInitialData;
    ZeroMemory(&vertexBufferInitialData, sizeof(D3D11_SUBRESOURCE_DATA));
    vertexBufferInitialData.pSysMem = vertices;

    {
        pin_ptr<ID3D11Buffer*> vertexBuffer = &_vertexBuffer;
        hResult = _device->CreateBuffer(&vertexBufferDescription, &vertexBufferInitialData, vertexBuffer);
        CHECKHR(hResult);

        UINT stride = sizeof(Vertex), offset = 0;
        _deviceContext->IASetVertexBuffers(0, 1, vertexBuffer, &stride, &offset);
    }

    _deviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D11_SAMPLER_DESC samplerDescription;
    ZeroMemory(&samplerDescription, sizeof(D3D11_SAMPLER_DESC));
    samplerDescription.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDescription.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDescription.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDescription.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDescription.MinLOD = 0;
    samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;

    ID3D11SamplerState* sampler = nullptr;
    hResult = _device->CreateSamplerState(&samplerDescription, &sampler);
    CHECKHR(hResult);

    _deviceContext->PSSetSamplers(0, 1, &sampler);

    ID3DBlob* vertexShaderBlob = nullptr;
    hResult = CompileShader(shader, "VS", "vs_4_0", &vertexShaderBlob);
    CHECKHR(hResult);

    ID3D11VertexShader* vertexShader = nullptr;
    hResult = _device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &vertexShader);
    CHECKHR(hResult);

    _deviceContext->VSSetShader(vertexShader, nullptr, 0);

    ID3DBlob* pixelShaderBlob = nullptr;
    hResult = CompileShader(shader, "PS", "ps_4_0", &pixelShaderBlob);
    CHECKHR(hResult);

    ID3D11PixelShader* pixelShader = nullptr;
    hResult = _device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &pixelShader);
    CHECKHR(hResult);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    _deviceContext->PSSetShader(pixelShader, nullptr, 0);

    ID3D11InputLayout* inputLayout = nullptr;
    hResult = _device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &inputLayout);
    CHECKHR(hResult);

    _deviceContext->IASetInputLayout(inputLayout);

CLEANUP:
    return hResult;
}

HRESULT CaptureManager::InitializeD3D9(UINT width, UINT height)
{
    IDirect3D9Ex* d3d9 = nullptr;
    IDirect3DDevice9Ex* device = nullptr;

    HRESULT hResult = Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9);
    CHECKHR(hResult);
    _d3d9 = d3d9;

    WNDCLASSEX windowClass;
    ZeroMemory(&windowClass, sizeof(WNDCLASSEX));
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = 0;
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = nullptr;
    windowClass.hIcon = nullptr;
    windowClass.hCursor = nullptr;
    windowClass.hbrBackground = nullptr;
    windowClass.lpszMenuName = nullptr;
    windowClass.lpszClassName = TEXT("HiddenWindowClassEx");
    windowClass.hIconSm = nullptr;

    if (!RegisterClassEx(&windowClass))
    {
        hResult = E_FAIL;
    }
    CHECKHR(hResult);

    HWND hwnd = CreateWindowEx(0, TEXT("HiddenWindowClassEx"), TEXT("HiddenWindow"), 0, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);

    D3DPRESENT_PARAMETERS presentParameters;
    ZeroMemory(&presentParameters, sizeof(D3DPRESENT_PARAMETERS));
    presentParameters.BackBufferWidth = 1;
    presentParameters.BackBufferHeight = 1;
    presentParameters.BackBufferCount = 1;
    presentParameters.Windowed = true;
    presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
    presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    hResult = d3d9->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &presentParameters, nullptr, &device);
    CHECKHR(hResult);
    _d3d9device = device;

CLEANUP:
    return hResult;
}

HRESULT CaptureManager::InitializeSurfaceQueue(UINT width, UINT height)
{
    SHARED_SURFACE_QUEUE_DESC surfaceDescription;
    ZeroMemory(&surfaceDescription, sizeof(SHARED_SURFACE_QUEUE_DESC));
    surfaceDescription.Width = width;
    surfaceDescription.Height = height;
    surfaceDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    surfaceDescription.NumSurfaces = 1;
    surfaceDescription.MetaDataSize = sizeof(int);
    surfaceDescription.Flags = SURFACE_QUEUE_FLAG_SINGLE_THREADED;

    ISurfaceQueue* surfaceQueue = nullptr;
    HRESULT hResult = CreateSurfaceQueue(&surfaceDescription, _d3d9device, &surfaceQueue);
    CHECKHR(hResult);
    _ABSurfaceQueue = surfaceQueue;

    SHARED_SURFACE_QUEUE_CLONE_DESC cloneDescription;
    cloneDescription.MetaDataSize = 0;
    cloneDescription.Flags = SURFACE_QUEUE_FLAG_SINGLE_THREADED;

    hResult = _ABSurfaceQueue->Clone(&cloneDescription, &surfaceQueue);
    CHECKHR(hResult);
    _BASurfaceQueue = surfaceQueue;

    ISurfaceConsumer* consumer = nullptr;
    hResult = _ABSurfaceQueue->OpenConsumer(_device, &consumer);
    CHECKHR(hResult);
    _ABSurfaceConsumer = consumer;

    ISurfaceProducer* producer = nullptr;
    hResult = _ABSurfaceQueue->OpenProducer(_d3d9device, &producer);
    CHECKHR(hResult);
    _ABSurfaceProducer = producer;

    hResult = _BASurfaceQueue->OpenConsumer(_d3d9device, &consumer);
    CHECKHR(hResult);
    _BASurfaceConsumer = consumer;

    hResult = _BASurfaceQueue->OpenProducer(_device, &producer);
    CHECKHR(hResult);
    _BASurfaceProducer = producer;

CLEANUP:
    return hResult;
}

CaptureManager::CaptureManager(int width, int height, bool useAlpha, int desiredSamplesCount)
{

    HRESULT hResult = InitializeD3D11(width, height);
    if (FAILED(hResult))
    {
        throw gcnew Exception("RenderingManager creation failed: " + hResult);
    }

    hResult = InitializeD3D9(width, height);
    if (FAILED(hResult))
    {
        throw gcnew Exception("RenderingManager creation failed: " + hResult);
    }

    hResult = InitializeSurfaceQueue(width, height);
    if (FAILED(hResult))
    {
        throw gcnew Exception("RenderingManager creation failed: " + hResult);
    }
}

CaptureManager::~CaptureManager()
{
}

void CaptureManager::Render(D3DImage^ d3dImage)
{
    d3dImage->Lock();

    _ABSurfaceProducer->Flush(0, nullptr);

    IDXGISurface* dxgiSurface = nullptr;
    IUnknown* dxgiSurfaceUnknown = nullptr;
    int count = 0;
    UINT size = sizeof(int);

    HRESULT hResult = _ABSurfaceConsumer->Dequeue(__uuidof(IDXGISurface), &dxgiSurfaceUnknown, &count, &size, INFINITE);
    CHECKHR(hResult);

    hResult = dxgiSurfaceUnknown->QueryInterface(IID_PPV_ARGS(&dxgiSurface));
    CHECKHR(hResult);

    if (_renderTarget == nullptr)
    {
        IDXGIResource* resource = nullptr;
        hResult = dxgiSurface->QueryInterface(IID_PPV_ARGS(&resource));
        CHECKHR(hResult);

        HANDLE sharedHandle;
        resource->GetSharedHandle(&sharedHandle);
        CHECKHR(hResult);

        resource->Release();

        IUnknown* tempResource = nullptr;
        hResult = _device->OpenSharedResource(sharedHandle, __uuidof(ID3D11Resource), (void**)&tempResource);
        CHECKHR(hResult);

        ID3D11Resource* d3d11Resource = nullptr;
        hResult = tempResource->QueryInterface(IID_PPV_ARGS(&d3d11Resource));
        CHECKHR(hResult);

        D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
        rtDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtDesc.Texture2D.MipSlice = 0;

        ID3D11RenderTargetView* renderTarget = nullptr;
        hResult = _device->CreateRenderTargetView(d3d11Resource, &rtDesc, &renderTarget);
        CHECKHR(hResult);
        _renderTarget = renderTarget;

        _deviceContext->OMSetRenderTargets(1, &renderTarget, nullptr);
    }

    /*float r = (rand() % 100 + 1) / 100.f;
    float g = (rand() % 100 + 1) / 100.f;
    float b = (rand() % 100 + 1) / 100.f;
    float color[] = { r, g, b, 1.0f };
    _deviceContext->ClearRenderTargetView(_renderTarget, color);*/

    IDXGIResource* frame = nullptr;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    ZeroMemory(&frameInfo, sizeof(DXGI_OUTDUPL_FRAME_INFO));
    hResult = _outputDuplication->AcquireNextFrame(INFINITE, &frameInfo, &frame);
    CHECKHR(hResult);

    ID3D11Resource* d3d11resource = nullptr;
    hResult = frame->QueryInterface(IID_PPV_ARGS(&d3d11resource));
    CHECKHR(hResult);

    ID3D11ShaderResourceView* shaderResourceView = nullptr;
    hResult = _device->CreateShaderResourceView(d3d11resource, nullptr, &shaderResourceView);
    CHECKHR(hResult);

    _deviceContext->PSSetShaderResources(0, 1, &shaderResourceView);

    _deviceContext->Draw(6, 0);

    _deviceContext->Flush();

    hResult = _outputDuplication->ReleaseFrame();
    CHECKHR(hResult);

    hResult = _BASurfaceProducer->Enqueue(dxgiSurface, nullptr, 0, SURFACE_QUEUE_FLAG_DO_NOT_WAIT);

    hResult = _BASurfaceProducer->Flush(0, nullptr);

    IUnknown* d3d9surfaceUnknown = nullptr;
    IDirect3DTexture9* d3d9texture = nullptr;
    IDirect3DSurface9* d3d9surface = nullptr;
    hResult = _BASurfaceConsumer->Dequeue(__uuidof(IDirect3DTexture9), &d3d9surfaceUnknown, nullptr, nullptr, INFINITE);
    CHECKHR(hResult);

    hResult = d3d9surfaceUnknown->QueryInterface(IID_PPV_ARGS(&d3d9texture));
    CHECKHR(hResult);

    hResult = d3d9texture->GetSurfaceLevel(0, &d3d9surface);
    CHECKHR(hResult);

    d3dImage->SetBackBuffer(D3DResourceType::IDirect3DSurface9, (IntPtr)d3d9surface);
    d3dImage->AddDirtyRect(Int32Rect(0, 0, d3dImage->PixelWidth, d3dImage->PixelHeight));

    hResult = _ABSurfaceProducer->Enqueue(d3d9texture, &count, sizeof(int), SURFACE_QUEUE_FLAG_DO_NOT_WAIT);

    hResult = _ABSurfaceProducer->Flush(SURFACE_QUEUE_FLAG_DO_NOT_WAIT, nullptr);

CLEANUP:
    d3dImage->Unlock();
}