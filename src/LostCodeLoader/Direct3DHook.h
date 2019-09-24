#pragma once

typedef HRESULT (Create9Ex_t)(UINT SDKVersion, void** dev);

extern HMODULE hD3D;
void HookDirectX();