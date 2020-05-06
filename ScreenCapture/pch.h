#pragma once

#include <windows.h>

#include <comdef.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <d3dx9math.h>

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

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
#define CHECKOUTOFMEMORY(x) if ((x) == NULL) CHECKHR(E_OUTOFMEMORY);


template <class T> void SafeRelease(T * *ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}