#pragma once

namespace ScreenCapture
{
    class __declspec(uuid("{840B536F-B6C5-4866-A102-07B2F12D68B7}")) ISurfaceProducer : public IUnknown
    {
    public:
        virtual HRESULT Enqueue(_In_ IUnknown* surface, _In_ void* buffer, _In_ UINT bufferSize, _In_ DWORD flags) = 0;
        virtual HRESULT Flush(_In_ DWORD flags, _Out_ UINT* surfaces) = 0;
    };
}
