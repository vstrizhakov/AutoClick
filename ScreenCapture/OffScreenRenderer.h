#pragma once
#include "Renderer.h"

namespace ScreenCapture
{
    class OffScreenRenderer : public Renderer
    {
    public:
        static HRESULT Create(IDirect3D9* d3d, IDirect3D9Ex* d3dEx, HWND hwnd, UINT adapter, Renderer** renderer);
        ~OffScreenRenderer();

        HRESULT Render();

    protected:
        HRESULT Init(IDirect3D9* d3d, IDirect3D9Ex* d3dEx, HWND hwnd, UINT adapter);

    private:
        OffScreenRenderer();

        IDirect3DSurface9* _offscreenSurface = NULL;
        IDirect3DVertexBuffer9* _vertexBuffer = NULL;
        IDirect3DTexture9* _texture = NULL;
    };
}