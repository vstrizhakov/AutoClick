#include "pch.h"
#include "DXGISurfaceQueue.h"
#include "RenderOutput.h"
using namespace System::Runtime::InteropServices;

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

const wchar_t* WindowName = TEXT("HiddenWindowClassEx");

HRESULT CompileShader(const char* source, LPCSTR entryPoint, LPCSTR profile, ID3D10Blob** blob);

RenderOutput::RenderOutput(ID3D11Device* d3d11Device, ID3D11DeviceContext* d3d11DeviceContext, D3DImage^ d3dImage, Size^ frameSize, Rect^ window)
{
    HRESULT hResult = S_OK;
    
    UINT width = frameSize->Width * window->Width;
    UINT height = frameSize->Height * window->Height;

    _d3d11Device = d3d11Device;
    _d3d11DeviceContext = d3d11DeviceContext;

    _d3dImage = d3dImage;
    _frameSize = frameSize;
    _window = window;

    hResult = InitializeD3D11();
    CHECKHR(hResult);

    hResult = InitializeD3D9();
    CHECKHR(hResult);

    hResult = InitializeSurfaceQueue(width, height);
    CHECKHR(hResult);

CLEANUP:
    if (FAILED(hResult))
    {
        throw gcnew Exception("RenderOutput creation failed: " + hResult);
    }
}

RenderOutput::~RenderOutput()
{

}

HRESULT RenderOutput::InitializeD3D11()
{
    HRESULT hResult = S_OK;

    /*{
        D3D_FEATURE_LEVEL* featureLevels = new D3D_FEATURE_LEVEL[1]
        {
            D3D_FEATURE_LEVEL_11_0,
        };

        UINT createFlags = 0;
#ifdef _DEBUG
        createFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

        pin_ptr<ID3D11Device*> device = &_d3d11Device;
        pin_ptr<ID3D11DeviceContext*> deviceContext = &_d3d11DeviceContext;
        hResult = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags, featureLevels, 1, D3D11_SDK_VERSION, device, nullptr, deviceContext);
        CHECKHR(hResult);
    }*/

    {
        pin_ptr<ID3D11Texture2D*> texture = &_d3d11texture;
        D3D11_TEXTURE2D_DESC textureDescription;
        ZeroMemory(&textureDescription, sizeof(D3D11_TEXTURE2D_DESC));
        textureDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        textureDescription.Width = _frameSize->Width * _window->Width;
        textureDescription.Height = _frameSize->Height * _window->Height;
        textureDescription.ArraySize = 1;
        textureDescription.BindFlags = 0;
        textureDescription.MipLevels = 1;
        textureDescription.MiscFlags = 0;
        textureDescription.SampleDesc.Count = 1;
        textureDescription.SampleDesc.Quality = 0;
        textureDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
        textureDescription.Usage = D3D11_USAGE_STAGING;
        hResult = _d3d11Device->CreateTexture2D(&textureDescription, nullptr, texture);
        CHECKHR(hResult);
    }
    Vertex vertices[] =
    {
        Vertex(-1.f, -1.f, 1.f, _window->Left, _window->Bottom),
        Vertex(-1.f,  1.f, 1.f, _window->Left, _window->Top),
        Vertex(1.f,  1.f, 1.f, _window->Right, _window->Top),
        Vertex(1.f,  1.f, 1.f, _window->Right, _window->Top),
        Vertex(1.f, -1.f, 1.f, _window->Right, _window->Bottom),
        Vertex(-1.f, -1.f, 1.f, _window->Left, _window->Bottom),
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
        hResult = _d3d11Device->CreateBuffer(&vertexBufferDescription, &vertexBufferInitialData, vertexBuffer);
        CHECKHR(hResult);

        UINT stride = sizeof(Vertex), offset = 0;
        _d3d11DeviceContext->IASetVertexBuffers(0, 1, vertexBuffer, &stride, &offset);
    }

    _d3d11DeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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
    hResult = _d3d11Device->CreateSamplerState(&samplerDescription, &sampler);
    CHECKHR(hResult);

    _d3d11DeviceContext->PSSetSamplers(0, 1, &sampler);

    ID3DBlob* vertexShaderBlob = nullptr;
    hResult = CompileShader(shader, "VS", "vs_4_0", &vertexShaderBlob);
    CHECKHR(hResult);

    ID3D11VertexShader* vertexShader = nullptr;
    hResult = _d3d11Device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &vertexShader);
    CHECKHR(hResult);

    _d3d11DeviceContext->VSSetShader(vertexShader, nullptr, 0);

    ID3DBlob* pixelShaderBlob = nullptr;
    hResult = CompileShader(shader, "PS", "ps_4_0", &pixelShaderBlob);
    CHECKHR(hResult);

    ID3D11PixelShader* pixelShader = nullptr;
    hResult = _d3d11Device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &pixelShader);
    CHECKHR(hResult);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    _d3d11DeviceContext->PSSetShader(pixelShader, nullptr, 0);

    ID3D11InputLayout* inputLayout = nullptr;
    hResult = _d3d11Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &inputLayout);
    CHECKHR(hResult);

    _d3d11DeviceContext->IASetInputLayout(inputLayout);

CLEANUP:
    return hResult;
}

HRESULT RenderOutput::InitializeD3D9()
{
    HRESULT hResult = S_OK;
    IDirect3DDevice9Ex* device = nullptr;

    {
        pin_ptr< IDirect3D9Ex*> d3d9 = &_d3d9;
        hResult = Direct3DCreate9Ex(D3D_SDK_VERSION, d3d9);
        CHECKHR(hResult);
    }

    {
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
        windowClass.lpszClassName = WindowName;
        windowClass.hIconSm = nullptr;

        if (!RegisterClassEx(&windowClass))
        {
            hResult = E_FAIL;
        }
        CHECKHR(hResult);

        HWND hwnd = CreateWindowEx(0, WindowName, WindowName, 0, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);

        D3DPRESENT_PARAMETERS presentParameters;
        ZeroMemory(&presentParameters, sizeof(D3DPRESENT_PARAMETERS));
        presentParameters.BackBufferWidth = 1;
        presentParameters.BackBufferHeight = 1;
        presentParameters.BackBufferCount = 1;
        presentParameters.Windowed = true;
        presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
        presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

        pin_ptr<IDirect3DDevice9Ex*> d3d9Device = &_d3d9Device;
        hResult = _d3d9->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &presentParameters, nullptr, d3d9Device);
        CHECKHR(hResult);
    }

CLEANUP:
    return hResult;
}

HRESULT RenderOutput::InitializeSurfaceQueue(UINT width, UINT height)
{
    HRESULT hResult = S_OK;

    {
        SHARED_SURFACE_QUEUE_DESC surfaceDescription;
        ZeroMemory(&surfaceDescription, sizeof(SHARED_SURFACE_QUEUE_DESC));
        surfaceDescription.Width = width;
        surfaceDescription.Height = height;
        surfaceDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        surfaceDescription.NumSurfaces = 1;
        surfaceDescription.MetaDataSize = sizeof(int);
        surfaceDescription.Flags = 0;

        pin_ptr<ISurfaceQueue*> surfaceQueue = &_surfaceQueueAB;
        hResult = CreateSurfaceQueue(&surfaceDescription, _d3d9Device, surfaceQueue);
        CHECKHR(hResult);
    }

    {
        SHARED_SURFACE_QUEUE_CLONE_DESC cloneDescription;
        cloneDescription.MetaDataSize = 0;
        cloneDescription.Flags = 0;

        pin_ptr<ISurfaceQueue*> surfaceQueue = &_surfaceQueueBA;
        hResult = _surfaceQueueAB->Clone(&cloneDescription, surfaceQueue);
        CHECKHR(hResult);
    }

    {
        pin_ptr<ISurfaceConsumer*> consumer = &_surfaceConsumerAB;
        hResult = _surfaceQueueAB->OpenConsumer(_d3d11Device, consumer);
        CHECKHR(hResult);
    }

    {
        pin_ptr<ISurfaceProducer*> producer = &_surfaceProducerAB;
        hResult = _surfaceQueueAB->OpenProducer(_d3d9Device, producer);
        CHECKHR(hResult);
    }

    {
        pin_ptr<ISurfaceConsumer*> consumer = &_surfaceConsumerBA;
        hResult = _surfaceQueueBA->OpenConsumer(_d3d9Device, consumer);
        CHECKHR(hResult);
    }

    {
        pin_ptr<ISurfaceProducer*> producer = &_surfaceProducerBA;
        hResult = _surfaceQueueBA->OpenProducer(_d3d11Device, producer);
        CHECKHR(hResult);
    }

CLEANUP:
    return hResult;
}

HRESULT RenderOutput::DrawFrame(ID3D11Resource* frame)
{
    {
        ID3D11Resource* d3d11frame = _d3d11frame;
        SafeRelease(&d3d11frame);
    }
    _d3d11frame = frame;
    _d3d11frame->AddRef();

    _d3dImage->Lock();

    _surfaceProducerAB->Flush(0, nullptr);

    IDXGISurface* dxgiSurface = nullptr;
    IUnknown* dxgiSurfaceUnknown = nullptr;
    int count = 0;
    UINT size = sizeof(int);

    HRESULT hResult = _surfaceConsumerAB->Dequeue(__uuidof(IDXGISurface), &dxgiSurfaceUnknown, &count, &size, INFINITE);
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
        hResult = _d3d11Device->OpenSharedResource(sharedHandle, __uuidof(ID3D11Resource), (void**)&tempResource);
        CHECKHR(hResult);

        ID3D11Resource* d3d11Resource = nullptr;
        hResult = tempResource->QueryInterface(IID_PPV_ARGS(&d3d11Resource));
        CHECKHR(hResult);

        D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
        rtDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtDesc.Texture2D.MipSlice = 0;

        ID3D11RenderTargetView* renderTarget = nullptr;
        hResult = _d3d11Device->CreateRenderTargetView(d3d11Resource, &rtDesc, &renderTarget);
        CHECKHR(hResult);
        _renderTarget = renderTarget;

        _d3d11DeviceContext->OMSetRenderTargets(1, &renderTarget, nullptr);
    }

    ID3D11ShaderResourceView* shaderResourceView = nullptr;
    hResult = _d3d11Device->CreateShaderResourceView(frame, nullptr, &shaderResourceView);
    CHECKHR(hResult);

    _d3d11DeviceContext->PSSetShaderResources(0, 1, &shaderResourceView);

    _d3d11DeviceContext->Draw(6, 0);

    _d3d11DeviceContext->Flush();

    hResult = _surfaceProducerBA->Enqueue(dxgiSurface, nullptr, 0, SURFACE_QUEUE_FLAG_DO_NOT_WAIT);

    hResult = _surfaceProducerBA->Flush(0, nullptr);

    IUnknown* d3d9surfaceUnknown = nullptr;
    IDirect3DTexture9* d3d9texture = nullptr;
    IDirect3DSurface9* d3d9surface = nullptr;
    hResult = _surfaceConsumerBA->Dequeue(__uuidof(IDirect3DTexture9), &d3d9surfaceUnknown, nullptr, nullptr, INFINITE);
    CHECKHR(hResult);

    hResult = d3d9surfaceUnknown->QueryInterface(IID_PPV_ARGS(&d3d9texture));
    CHECKHR(hResult);

    hResult = d3d9texture->GetSurfaceLevel(0, &d3d9surface);
    CHECKHR(hResult);

    _d3dImage->SetBackBuffer(D3DResourceType::IDirect3DSurface9, (IntPtr)d3d9surface);
    _d3dImage->AddDirtyRect(Int32Rect(0, 0, _d3dImage->PixelWidth, _d3dImage->PixelHeight));

    hResult = _surfaceProducerAB->Enqueue(d3d9texture, &count, sizeof(int), SURFACE_QUEUE_FLAG_DO_NOT_WAIT);

    hResult = _surfaceProducerAB->Flush(SURFACE_QUEUE_FLAG_DO_NOT_WAIT, nullptr);

CLEANUP:
    _d3dImage->Unlock();
    return hResult;
}

array<byte>^ RenderOutput::GetData()
{
    HRESULT hResult = S_OK;

    UINT width = _frameSize->Width * _window->Width;
    UINT height = _frameSize->Height * _window->Height;
    D3D11_BOX box = { 0, 0, 0, width, height, 1 };
    _d3d11DeviceContext->CopySubresourceRegion(_d3d11texture, 0, 0, 0, 0, _d3d11frame, 0, &box);

    D3D11_MAPPED_SUBRESOURCE map;
    hResult = _d3d11DeviceContext->Map(_d3d11texture, 0, D3D11_MAP_READ, 0, &map);
    CHECKHR(hResult);

    void* data = map.pData;
    UINT pitch = map.RowPitch;
    UINT dataSize = pitch * height;
    array<byte>^ bytes = gcnew array<byte>(dataSize);
    Marshal::Copy((IntPtr)data, bytes, 0, dataSize);

    _d3d11DeviceContext->Unmap(_d3d11texture, 0);

CLEANUP:
    return bytes;
}

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