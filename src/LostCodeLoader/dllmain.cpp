// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "Direct3DHook.h"
#include "fstream"
#include "configuration.h"
#include "LostCodeLoader.h"
#include "Events.h"
#include "helpers.h"
#include <CodeParser.hpp>
#include <Unknwn.h>

using namespace std;
ModInfo* ModsInfo;
CodeParser codeParser;
intptr_t BaseAddress = (intptr_t)GetModuleHandle(nullptr);

class IDirect3D9;
class IDirect3DDevice9;

IDirect3DDevice9* Device;

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

VTABLE_HOOK(void, __stdcall, IDirect3DDevice9, EndScene)
{
	RaiseEvents(modFrameEvents);
	codeParser.processCodeList(false);
	originalEndScene(This);
}

VTABLE_HOOK(HRESULT, __stdcall, IUnknown, CreateDevice, UINT Adapter, void* DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, void* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
	HRESULT result = originalCreateDevice(This, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
	Device = *ppReturnedDeviceInterface;
	INSTALL_VTABLE_HOOK(Device, EndScene, 42);
	return result;
}

#pragma endregion

void DeviceCreateEvent(DWORD* device)
{
	INSTALL_VTABLE_HOOK((IUnknown*)device, CreateDevice, 16);
}

void InitLoader()
{
	INSTALL_HOOK(SteamAPI_RestartAppIfNecessary);
	INSTALL_HOOK(SteamAPI_IsSteamRunning);

	ModsInfo = new ModInfo();
	ModsInfo->ModList = new vector<Mod*>();
	vector<ModInitEvent> postEvents;

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
	
	vector<string*> strings;
	int count = modsdb.getInt("Main", "ActiveModCount");
	for (int i = 0; i < count; i++)
	{
		ConfigurationFile modConfig;
		string name = modsdb.getString("Main", "ActiveMod" + to_string(i));
		string path = modsdb.getString("Mods", name);
		string dir = path.substr(0, path.find_last_of("\\")) + "\\";
		if (ConfigurationFile::open(path, &modConfig))
		{
			auto mod = new Mod();
			auto modTitle = new string(modConfig.getString("Desc", "Title"));
			auto modPath = new string(path);
			mod->Name = modTitle->c_str();
			mod->Path = modPath->c_str();
			strings.push_back(modTitle);
			strings.push_back(modPath);
			ModsInfo->CurrentMod = mod;
			ModsInfo->ModList->push_back(mod);
			string dllName = modConfig.getString("Main", "DLLFile");
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

	for (ModInitEvent event : postEvents)
		event(ModsInfo);

	for (auto string : strings)
		delete string;

	// Create a Direct3D device and hook it's vtable
	D3DCreateEvent = &DeviceCreateEvent;
}

static const char VersionCheck2[] = { 0xE8u, 0xE8u, 0x0C, 0x02, 0x00 };

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		HookDirectX();
		if (!memcmp(VersionCheck2, (const char*)(BaseAddress + 0x00001073), sizeof(VersionCheck2)))
			break; // Config Tool

		InitLoader();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
		RaiseEvents(modExitEvents);
        break;
    }
    return TRUE;
}

