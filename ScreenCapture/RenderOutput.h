#pragma once
#include "ISurfaceQueue.h"

using namespace System::Windows;
using namespace System::Windows::Interop;

namespace ScreenCapture
{
    public ref class RenderOutput
    {
    public:
        ~RenderOutput();
        array<byte>^ GetData();

    internal:
        RenderOutput(ID3D11Device* d3d11Device, ID3D11DeviceContext* d3d11DeviceContext, D3DImage^ d3dImage, Size^ frameSize, Rect^ window);

        HRESULT DrawFrame(ID3D11Resource* frame);

    private:
        HRESULT InitializeD3D9();
        HRESULT InitializeD3D11();
        HRESULT InitializeSurfaceQueue(UINT width, UINT height);

    private:
        D3DImage^ _d3dImage = nullptr;
        Size^ _frameSize = nullptr;
        Rect^ _window = nullptr;

        ISurfaceQueue* _surfaceQueueAB = nullptr;
        ISurfaceQueue* _surfaceQueueBA = nullptr;
        ISurfaceProducer* _surfaceProducerAB = nullptr;
        ISurfaceConsumer* _surfaceConsumerAB = nullptr;
        ISurfaceProducer* _surfaceProducerBA = nullptr;
        ISurfaceConsumer* _surfaceConsumerBA = nullptr;

        ID3D11Device* _d3d11Device = nullptr;
        ID3D11DeviceContext* _d3d11DeviceContext = nullptr;
        ID3D11Buffer* _vertexBuffer = nullptr;
        ID3D11RenderTargetView* _renderTarget = nullptr;

        IDirect3D9Ex* _d3d9 = nullptr;
        IDirect3DDevice9Ex* _d3d9Device = nullptr;

        ID3D11Resource* _d3d11frame = nullptr;
        ID3D11Texture2D* _d3d11texture = nullptr;
    };
}
