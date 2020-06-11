
#define INI_MAX_LINE 2000

#include "pch.h"
#include "Direct3DHook.h"
#include "fstream"
#include "LostCodeLoader.h"
#include "Events.h"
#include "helpers.h"
#include <Unknwn.h>
#include "sigscanner.h"
#include "INIReader.h"
#include <CommonLoader.h>

using namespace std;
ModInfo* ModsInfo;
intptr_t BaseAddress = (intptr_t)GetModuleHandle(nullptr);

DWORD OnFrameStubAddress = 0;

class IDirect3D9Ex;
class IDirect3DDevice9Ex;

void InitMods();

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

HOOK(int, _cdecl, ProcessStart, PROCESS_ENTRY)
{
	InitMods();
	return originalProcessStart();
}

HOOK(void, _cdecl, SteamAPI_Shutdown, PROC_ADDRESS("steam_api.dll", "SteamAPI_Shutdown"))
{
	RaiseEvents(modExitEvents);
	originalSteamAPI_Shutdown();
}

#pragma endregion

#pragma region DirectX Vtable hooks

VTABLE_HOOK(void, __stdcall, IDirect3DDevice9Ex, EndScene)
{
	RaiseEvents(modFrameEvents);
	CommonLoader::CommonLoader::RaiseUpdates();
	originalEndScene(This);
}

VTABLE_HOOK(HRESULT, __stdcall, IDirect3D9Ex, CreateDeviceEx, UINT Adapter, DWORD DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, void* pPresentationParameters, void* pFullscreenDisplayMode, IDirect3DDevice9Ex** ppReturnedDeviceInterface)
{
	HRESULT result = originalCreateDeviceEx(This, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, pFullscreenDisplayMode, ppReturnedDeviceInterface);
	INSTALL_VTABLE_HOOK(*ppReturnedDeviceInterface, EndScene, 42);
	return result;
}

#pragma endregion

void DeviceCreateEvent(void* d3d)
{
	INSTALL_VTABLE_HOOK(*((IDirect3D9Ex**)d3d), CreateDeviceEx, 20);
}

void InitLoader()
{
	INSTALL_HOOK(SteamAPI_RestartAppIfNecessary);
	INSTALL_HOOK(SteamAPI_IsSteamRunning);
	INSTALL_HOOK(ProcessStart);
	INSTALL_HOOK(SteamAPI_Shutdown);
}

void InitMods()
{
	ModsInfo = new ModInfo();
	ModsInfo->ModList = new vector<Mod*>();
	vector<ModInitEvent> postEvents;

	char pathbuf[MAX_PATH];
	GetModuleFileNameA(NULL, pathbuf, MAX_PATH);

	string exePath(pathbuf);
	string exeDir = exePath.substr(0, exePath.find_last_of("\\"));

	INIReader cpkredir(exeDir + "\\cpkredir.ini");
	string modsPath = cpkredir.GetString("CPKREDIR", "ModsDbIni", "mods\\ModsDb.ini");
	string modsDir = modsPath.substr(0, modsPath.find_last_of("\\"));
	INIReader modsdb(modsPath);

	if (cpkredir.GetBoolean("CPKREDIR", "Enabled", true))
	{
		LoadLibraryA("cpkredir.dll");
	}

	CommonLoader::CommonLoader::InitializeAssemblyLoader((modsDir + "\\Codes.dll").c_str());
	CommonLoader::CommonLoader::RaiseInitializers();

	vector<string*> strings;
	int count = modsdb.GetInteger("Main", "ActiveModCount", 0);
	for (int i = 0; i < count; i++)
	{
		string name = modsdb.GetString("Main", "ActiveMod" + to_string(i), "");
		string path = modsdb.GetString("Mods", name, "");
		string dir = path.substr(0, path.find_last_of("\\")) + "\\";
		INIReader modConfig(path);

		if (modConfig.ParseError() == 0)
		{
			auto mod = new Mod();
			auto modTitle = new string(modConfig.GetString("Desc", "Title", ""));
			auto modPath = new string(path);
			mod->Name = modTitle->c_str();
			mod->Path = modPath->c_str();
			strings.push_back(modTitle);
			strings.push_back(modPath);
			ModsInfo->CurrentMod = mod;
			ModsInfo->ModList->push_back(mod);
			string dllName = modConfig.GetString("Main", "DLLFile", "");
			if (!dllName.empty())
			{
				printf("Loading %s\n", dllName.c_str());
				SetDllDirectoryA(dir.c_str());
				SetCurrentDirectoryA(dir.c_str());

				HMODULE module = LoadLibraryA((dir + dllName).c_str());
				if (module)
				{
					ModInitEvent init = (ModInitEvent)GetProcAddress(module, "Init");
					ModInitEvent postInit = (ModInitEvent)GetProcAddress(module, "PostInit");
					if (init)
					{
						init(ModsInfo);
					}
					if (postInit)
					{
						postEvents.push_back(postInit);
					}
					RegisterEvent(modFrameEvents, module, "OnFrame");
					RegisterEvent(modExitEvents, module, "OnExit");
					SetupD3DModuleHooks(module);
				}
			}
		}
	}

	SetCurrentDirectoryA(exeDir.c_str());

	for (ModInitEvent event : postEvents)
		event(ModsInfo);

	for (auto string : strings)
		delete string;

	D3DCreateEvent = &DeviceCreateEvent;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		HookDirectX();
		InitLoader();
        
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

