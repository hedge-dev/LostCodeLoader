#include "pch.h"
#include "Direct3DHook.h"
#include "windows.h"
#include "stdio.h"
#include <Unknwn.h>

#pragma warning(disable:6387)
#define EXPORT comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)

HMODULE hD3D;
void* DirectXFuncs[8];
DeviceCreateEvent_t* D3DCreateEvent;

void SetupD3DModuleHooks(HMODULE mod)
{
	void* method = GetProcAddress(mod, "D3DPERF_SetMarker");
	if (method)
		DirectXFuncs[0] = method;

	method = GetProcAddress(mod, "D3DPERF_GetStatus");
	if (method)
		DirectXFuncs[1] = method;

	method = GetProcAddress(mod, "D3DPERF_BeginEvent");
	if (method)
		DirectXFuncs[2] = method;

	method = GetProcAddress(mod, "D3DPERF_EndEvent");
	if (method)
		DirectXFuncs[3] = method;

	method = GetProcAddress(mod, "D3DPERF_QueryRepeatFrame");
	if (method)
		DirectXFuncs[4] = method;

	method = GetProcAddress(mod, "D3DPERF_SetOptions");
	if (method)
		DirectXFuncs[5] = method;

	method = GetProcAddress(mod, "Direct3DCreate9");
	if (method)
		DirectXFuncs[6] = method;

	method = GetProcAddress(mod, "Direct3DCreate9Ex");
	if (method)
		DirectXFuncs[7] = method;
}

void HookDirectX()
{
	char windir[MAX_PATH];
	GetSystemDirectoryA(windir, MAX_PATH);
	char d3d[MAX_PATH];
	snprintf(d3d, MAX_PATH, "%s\\d3d9.dll", windir);
	hD3D = LoadLibraryA(d3d);
	SetupD3DModuleHooks(hD3D);
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

	_declspec(dllexport) __declspec(naked) DWORD* __stdcall Direct3DCreate9(UINT Version)
	{
#pragma EXPORT
		__asm jmp dword ptr[DirectXFuncs + 24]
	}

	__declspec(dllexport) HRESULT __stdcall Direct3DCreate9Ex(UINT Version, void* d3d)
	{
#pragma EXPORT
		HRESULT result = ((Create9Ex*)DirectXFuncs[7])(Version, d3d);

		if (D3DCreateEvent)
			D3DCreateEvent(d3d);

		return result;
	}
}