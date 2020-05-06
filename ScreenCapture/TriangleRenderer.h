#pragma once
#include "Renderer.h"

namespace ScreenCapture
{
    class TriangleRenderer : public Renderer
    {
    public:
        static HRESULT Create(IDirect3D9* d3d, IDirect3D9Ex* d3dEx, HWND hwnd, UINT adapter, Renderer** renderer);
        ~TriangleRenderer();

        HRESULT Render();

    protected:
        HRESULT Init(IDirect3D9* d3d, IDirect3D9Ex* d3dEx, HWND hwnd, UINT adapter);

    private:
        TriangleRenderer();

        IDirect3DVertexBuffer9* _vertexBuffer;
    };
}