#include "pch.h"
#include "Direct3DHook.h"
#include "windows.h"
#include "stdio.h"

#pragma warning(disable:6387)

void* DirectXFuncs[6];
HMODULE hD3D;

void HookDirectX()
{
	char windir[MAX_PATH];
	GetSystemDirectoryA(windir, MAX_PATH);
	char d3d[MAX_PATH];
	snprintf(d3d, MAX_PATH, "%s\\d3d9.dll", windir);
	hD3D = LoadLibraryA(d3d);
	DirectXFuncs[0] = GetProcAddress(hD3D, "D3DPERF_SetMarker");
	DirectXFuncs[1] = GetProcAddress(hD3D, "D3DPERF_GetStatus");
	DirectXFuncs[2] = GetProcAddress(hD3D, "D3DPERF_BeginEvent");
	DirectXFuncs[3] = GetProcAddress(hD3D, "D3DPERF_EndEvent");
	DirectXFuncs[4] = GetProcAddress(hD3D, "D3DPERF_QueryRepeatFrame");
	DirectXFuncs[5] = GetProcAddress(hD3D, "D3DPERF_SetOptions");
	DirectXFuncs[6] = GetProcAddress(hD3D, "Direct3DCreate9");
	DirectXFuncs[7] = GetProcAddress(hD3D, "Direct3DCreate9Ex");
}

extern "C"
{
	void __declspec(naked) __declspec(dllexport) D3DPERF_SetMarker()
	{
		__asm jmp dword ptr[DirectXFuncs]
	}

	void __declspec(naked) __declspec(dllexport) D3DPERF_GetStatus()
	{
		__asm jmp dword ptr[DirectXFuncs + 4]
	}

	void __declspec(naked) __declspec(dllexport) D3DPERF_BeginEvent()
	{
		__asm jmp dword ptr[DirectXFuncs + 8]
	}

	void __declspec(naked) __declspec(dllexport) D3DPERF_EndEvent()
	{
		__asm jmp dword ptr[DirectXFuncs + 12]
	}
	
	void __declspec(naked) __declspec(dllexport) D3DPERF_QueryRepeatFrame()
	{
		__asm jmp dword ptr[DirectXFuncs + 16]
	}
	
	void __declspec(naked) __declspec(dllexport) D3DPERF_SetOptions()
	{
		__asm jmp dword ptr[DirectXFuncs + 20]
	}

	void __declspec(naked) __declspec(dllexport) Direct3DCreate9()
	{
		__asm jmp dword ptr[DirectXFuncs + 24]
	}

	HRESULT __declspec(naked) __declspec(dllexport) Direct3DCreate9Ex()
	{
		__asm jmp dword ptr[DirectXFuncs + 28]
	}
}