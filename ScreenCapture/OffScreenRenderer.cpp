#include "pch.h"
#include "OffScreenRenderer.h"

using namespace ScreenCapture;

struct TextureVertex
{
    float x, y, z, rhw;
    float tu, tv;
};

#define TRI_FVF D3DFVF_XYZRHW | D3DFVF_TEX1

HRESULT OffScreenRenderer::Create(IDirect3D9* d3d, IDirect3D9Ex* d3dEx, HWND hwnd, UINT adapter, Renderer** renderer)
{
    HRESULT hResult = S_OK;

    OffScreenRenderer* offscreenRenderer = new OffScreenRenderer();
    CHECKOUTOFMEMORY(offscreenRenderer);

    hResult = offscreenRenderer->Init(d3d, d3dEx, hwnd, adapter);
    CHECKHR(hResult);

    *renderer = offscreenRenderer;
    offscreenRenderer = nullptr;

CLEANUP:
    delete offscreenRenderer;

    return hResult;
}

OffScreenRenderer::~OffScreenRenderer()
{
    SafeRelease(&_texture);
    SafeRelease(&_offscreenSurface);
}

OffScreenRenderer::OffScreenRenderer() : Renderer(), _offscreenSurface(NULL), _vertexBuffer(NULL)
{

}

HRESULT OffScreenRenderer::Init(IDirect3D9* d3d, IDirect3D9Ex* d3dEx, HWND hwnd, UINT adapter)
{
    HRESULT hResult = S_OK;

    hResult = Renderer::Init(d3d, d3dEx, hwnd, adapter);
    CHECKHR(hResult);

    D3DDISPLAYMODE displayMode;
    hResult = _device->GetDisplayMode(adapter, &displayMode);
    CHECKHR(hResult);

    /* Vertex buffer*/

    TextureVertex vertices[] =
    {
        { 0.0f,                 0.0f,                   0.0f, 1.0f, 0.0f, 0.0f, },
        { 0.0f,                 displayMode.Height,     0.0f, 1.0f, 0.0f, 1.0f, },
        { displayMode.Width,    0.0f,                   0.0f, 1.0f, 1.0f, 0.0f, },
        { displayMode.Width,    0.0f,                   0.0f, 1.0f, 1.0f, 0.0f, },
        { displayMode.Width,    displayMode.Height,     0.0f, 1.0f, 1.0f, 1.0f, },
        { 0.0f,                 displayMode.Height,     0.0f, 1.0f, 0.0f, 1.0f, },
    };

    hResult = _device->CreateVertexBuffer(sizeof(vertices), 0, TRI_FVF, D3DPOOL_DEFAULT, &_vertexBuffer, NULL);
    CHECKHR(hResult);

    void* pVertices = NULL;
    hResult = _vertexBuffer->Lock(0, sizeof(vertices), &pVertices, 0);
    CHECKHR(hResult);

    memcpy(pVertices, vertices, sizeof(vertices));

    hResult = _vertexBuffer->Unlock();
    CHECKHR(hResult);

    hResult = _device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    CHECKHR(hResult);
    hResult = _device->SetRenderState(D3DRS_LIGHTING, FALSE);
    CHECKHR(hResult);
    hResult = _device->SetStreamSource(0, _vertexBuffer, 0, sizeof(TextureVertex));
    CHECKHR(hResult);
    hResult = _device->SetFVF(TRI_FVF);
    CHECKHR(hResult);

    /* Offscreen surface */

    hResult = _device->CreateOffscreenPlainSurface(displayMode.Width, displayMode.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &_offscreenSurface, nullptr);
    CHECKHR(hResult);

    // TODO: Check caps - square texture and so on...
    hResult = D3DXCreateTexture(_device, displayMode.Width, displayMode.Height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &_texture);
    CHECKHR(hResult);

    hResult = _device->SetTexture(0, _texture);
    CHECKHR(hResult);

    hResult = _device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    hResult = _device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    hResult = _device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

CLEANUP:
    return hResult;
}

HRESULT OffScreenRenderer::Render()
{
    HRESULT hResult = S_OK;

    hResult = _device->GetFrontBufferData(0, _offscreenSurface);
    CHECKHR(hResult);

    D3DLOCKED_RECT surfaceRect, textureRect;
    hResult = _offscreenSurface->LockRect(&surfaceRect, NULL, 0);
    CHECKHR(hResult);

    hResult = _texture->LockRect(0, &textureRect, NULL, 0);
    CHECKHR(hResult);

    memcpy(textureRect.pBits, surfaceRect.pBits, surfaceRect.Pitch * 1080);

    hResult = _texture->UnlockRect(0);
    CHECKHR(hResult);

    hResult = _offscreenSurface->UnlockRect();
    CHECKHR(hResult);

    hResult = _device->BeginScene();
    CHECKHR(hResult);

    hResult = _device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(128, 0, 0, 128), 1.0f, 0);
    CHECKHR(hResult);

    hResult = _device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
    CHECKHR(hResult);

    hResult = _device->EndScene();
    CHECKHR(hResult);

CLEANUP:
    return hResult;
}