#pragma once
#define ML_API extern "C" __declspec(dllexport)

typedef DWORD* (__stdcall Create9)(UINT SDKVersion);
typedef void DeviceCreateEvent_t(DWORD* device);

extern HMODULE hD3D;
extern void* DirectXFuncs[8];
extern DeviceCreateEvent_t* D3DCreateEvent;

void HookDirectX();
void SetupD3DModuleHooks(HMODULE mod);

ML_API DWORD* __stdcall Direct3DCreate9(UINT Version);