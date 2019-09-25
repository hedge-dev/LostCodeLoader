// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "Direct3DHook.h"
#include "fstream"
#include "d3d9.h"
#include "configuration.h"
#include "LostCodeLoader.h"
#include "Events.h"
#include "helpers.h"
#include <CodeParser.hpp>

using namespace std;
ModInfo* ModsInfo;
CodeParser codeParser;

// Hook steam so we dont have to use that as launcher everytime
#pragma region Steam Hooks

HOOK(bool, __cdecl, SteamAPI_RestartAppIfNecessary, PROC_ADDRESS("steam_api.dll", "SteamAPI_RestartAppIfNecessary"), uint32_t appid)
{
	originalSteamAPI_RestartAppIfNecessary(appid);
	std::ofstream ofs("steam_appid.txt");
	ofs << appid;
	ofs.close();
	return false;
}

HOOK(bool, __cdecl, SteamAPI_IsSteamRunning, PROC_ADDRESS("steam_api.dll", "SteamAPI_IsSteamRunning"))
{
	originalSteamAPI_IsSteamRunning();
	return true;
}

#pragma endregion

#pragma region DirectX Vtable hooks

VTABLE_HOOK(void, __stdcall, IDirect3DDevice9Ex, EndScene)
{
	RaiseEvents(modFrameEvents);
	codeParser.processCodeList(false);
	originalEndScene(This);
}

VTABLE_HOOK(HRESULT, __stdcall, IDirect3D9Ex, CreateDeviceEx, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode, IDirect3DDevice9Ex** ppReturnedDeviceInterface)
{
	HRESULT result = originalCreateDeviceEx(This, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, pFullscreenDisplayMode, ppReturnedDeviceInterface);
	INSTALL_VTABLE_HOOK(*ppReturnedDeviceInterface, EndScene, 42);
	return result;
}

#pragma endregion

void InitLoader()
{
	INSTALL_HOOK(SteamAPI_RestartAppIfNecessary);
	INSTALL_HOOK(SteamAPI_IsSteamRunning);
	HookDirectX();

	IDirect3D9Ex* d3d;
	Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d);
	INSTALL_VTABLE_HOOK(d3d, CreateDeviceEx, 20);
	d3d->Release();

	ModsInfo = new ModInfo();
	ModsInfo->ModList = new vector<Mod*>();
	char pathbuf[MAX_PATH];
	GetModuleFileNameA(NULL, pathbuf, MAX_PATH);

	string exePath(pathbuf);
	string exeDir = exePath.substr(0, exePath.find_last_of("\\"));

	ConfigurationFile cpkredir;
	ConfigurationFile modsdb;
	ConfigurationFile::open(exeDir + "\\cpkredir.ini", &cpkredir);
	string modsPath = cpkredir.getString("CPKREDIR", "ModsDbIni", "mods\\ModsDb.ini");
	string modsDir = modsPath.substr(0, modsPath.find_last_of("\\"));
	ConfigurationFile::open(modsPath, &modsdb);

	if (cpkredir.getBool("CPKREDIR", "Enabled"))
	{
		LoadLibraryA("cpkredir.dll");
	}

	int count = modsdb.getInt("Main", "ActiveModCount");
	for (int i = 0; i < count; i++)
	{
		ConfigurationFile modConfig;
		string name = modsdb.getString("Main", "ActiveMod" + to_string(i));
		string path = modsdb.getString("Mods", name);
		string dir = path.substr(0, path.find_last_of("\\")) + "\\";
		auto mod = new Mod();
		mod->Name = name.c_str();
		mod->Path = path.c_str();
		ModsInfo->CurrentMod = mod;
		ModsInfo->ModList->push_back(mod);
		if (ConfigurationFile::open(path, &modConfig))
		{
			string dllName = modConfig.getString("Main", "DLLFile");
			if (dllName.size() > 0)
			{
				HMODULE module = LoadLibraryA((dir + dllName).c_str());
				if (module) 
				{
					ModInitEvent init = (ModInitEvent)GetProcAddress(module, "Init");
					if (init) 
					{
						init(ModsInfo);
					}
					RegisterEvent(modFrameEvents, module, "OnFrame");
					RegisterEvent(modExitEvents, module, "OnExit");
				}
			}
		}
	}

	ifstream codesStream(modsDir + "\\Codes.dat", ifstream::binary);
	if (codesStream.is_open())
	{
		int count = codeParser.readCodes(codesStream);
		if (count > 0)
		{
			codeParser.processCodeList(true);
		}
		codesStream.close();
	}
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		InitLoader();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
		RaiseEvents(modExitEvents);
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

