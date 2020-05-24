#pragma once

namespace ScreenCapture
{
    class __declspec(uuid("{F3B68DBF-D45D-467F-B112-BCB67C5AFDDA}")) ISurfaceConsumer : public IUnknown
    {
    public:
        virtual HRESULT Dequeue(_In_ REFIID id, _Out_ IUnknown** ppSurface, _In_ _Out_ void* pBuffer, _In_ _Out_ UINT* pBufferSize, _In_ DWORD timeout) = 0;
    };
}
