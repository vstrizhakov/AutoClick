#include "pch.h"
#include "TriangleRenderer.h"

using namespace ScreenCapture;
using namespace System;

struct CustomVertex
{
    FLOAT x, y, z;
    DWORD color;
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

TriangleRenderer::TriangleRenderer() : Renderer(), _vertexBuffer(NULL)
{

}

TriangleRenderer::~TriangleRenderer()
{
    SafeRelease(&_vertexBuffer);
}

HRESULT TriangleRenderer::Create(IDirect3D9* d3d, IDirect3D9Ex* d3dEx, HWND hwnd, UINT adapter, Renderer** renderer)
{
    HRESULT hResult = S_OK;

    TriangleRenderer* triangleRenderer = new TriangleRenderer();
    CHECKOUTOFMEMORY(triangleRenderer);

    hResult = triangleRenderer->Init(d3d, d3dEx, hwnd, adapter);
    CHECKHR(hResult);

    *renderer = triangleRenderer;
    triangleRenderer = nullptr;

CLEANUP:
    delete triangleRenderer;

    return hResult;
}

HRESULT TriangleRenderer::Init(IDirect3D9* d3d, IDirect3D9Ex* d3dEx, HWND hwnd, UINT adapter)
{
    HRESULT hResult = S_OK;

    D3DXMATRIXA16 matView, matProj;
    D3DXVECTOR3 eyePoint(0.0f, 0.0f, -5.0f);
    D3DXVECTOR3 lookAtPoint(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 upVector(0.0f, 1.0f, 0.0f);

    hResult = Renderer::Init(d3d, d3dEx, hwnd, adapter);

    CustomVertex vertices[] =
    {
        { -1.0f, -1.0f, 0.0f, 0xFFFF0000, },
        {  1.0f, -1.0f, 0.0f, 0xFF00FF00, },
        {  0.0f,  1.0f, 0.0f, 0xFF00FFFF, },
    };

    hResult = _device->CreateVertexBuffer(sizeof(vertices), 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &_vertexBuffer, NULL);

    void* pVertices;
    hResult = _vertexBuffer->Lock(0, sizeof(vertices), &pVertices, 0);
    CHECKHR(hResult);
    memcpy(pVertices, vertices, sizeof(vertices));
    hResult = _vertexBuffer->Unlock();
    CHECKHR(hResult);

    D3DXMatrixLookAtLH(&matView, &eyePoint, &lookAtPoint, &upVector);
    hResult = _device->SetTransform(D3DTS_VIEW, &matView);
    CHECKHR(hResult);
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f);
    hResult = _device->SetTransform(D3DTS_PROJECTION, &matProj);
    CHECKHR(hResult);

    hResult = _device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    CHECKHR(hResult);
    hResult = _device->SetRenderState(D3DRS_LIGHTING, FALSE);
    CHECKHR(hResult);
    hResult = _device->SetStreamSource(0, _vertexBuffer, 0, sizeof(CustomVertex));
    CHECKHR(hResult);
    hResult = _device->SetFVF(D3DFVF_CUSTOMVERTEX);
    CHECKHR(hResult);

CLEANUP:
    return hResult;
}

HRESULT TriangleRenderer::Render()
{
    HRESULT hResult = S_OK;

    D3DXMATRIXA16 matWorld;

    hResult = _device->BeginScene();
    CHECKHR(hResult);
    hResult = _device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(128, 0, 0, 128), 1.0f, 0);
    CHECKHR(hResult);

    UINT time = GetTickCount() % 10000;
    FLOAT angle = time * (2.0f * D3DX_PI) / 10000.0f;
    D3DXMatrixRotationY(&matWorld, angle);
    hResult = _device->SetTransform(D3DTS_WORLD, &matWorld);
    CHECKHR(hResult);

    hResult = _device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
    CHECKHR(hResult);

    hResult = _device->EndScene();
    CHECKHR(hResult);


CLEANUP:
    return hResult;
}