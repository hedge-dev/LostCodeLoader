#pragma once
#define ML_API extern "C" __declspec(dllexport)

typedef HRESULT (Create9Ex_t)(UINT SDKVersion, void** dev);

extern HMODULE hD3D;
extern void* DirectXFuncs[8];
void HookDirectX();
void SetupD3DModuleHooks(HMODULE mod);

ML_API DWORD* __stdcall Direct3DCreate9(UINT Version);