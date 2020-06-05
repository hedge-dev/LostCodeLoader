#pragma once
#define ML_API extern "C" __declspec(dllexport)

typedef HRESULT(__stdcall Create9Ex)(UINT SDKVersion, void* device);
typedef void DeviceCreateEvent_t(void* device);

extern HMODULE hD3D;
extern void* DirectXFuncs[8];
extern DeviceCreateEvent_t* D3DCreateEvent;

void HookDirectX();
void SetupD3DModuleHooks(HMODULE mod);

ML_API DWORD* __stdcall Direct3DCreate9(UINT Version);
ML_API HRESULT __stdcall Direct3DCreate9Ex(UINT Version, void* d3d);