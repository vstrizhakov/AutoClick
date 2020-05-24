#pragma once

#include <windows.h>

#include <d3d9.h>
#include <dxgi1_2.h>
#include <dxgi1_5.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <comdef.h>
#include <d3dx9.h>
#include <d3dx9math.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define PRINT_COM_ERROR(x) \
	_com_error err(x); \
	LPCTSTR errMsg = err.ErrorMessage(); \
	System::Diagnostics::Debug::WriteLine(gcnew System::String(errMsg)); 

#define HRCHECK(x) \
if (FAILED(x)) \
{ \
	return x; \
}

#define CHECKHR(hResult) if (FAILED(hResult)) goto CLEANUP;
#define CHECKOUTOFMEMORY(x) if ((x) == nullptr) CHECKHR(E_OUTOFMEMORY);


template <class T> void SafeRelease(T * *ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = nullptr;
	}
}