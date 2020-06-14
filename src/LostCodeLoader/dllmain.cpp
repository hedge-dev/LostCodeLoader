
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

const char* OnFrameStubSignature = "\x56\x8B\xF1\x8D\x86\x48\x76\x00\x00\x50\xE8\x61\xE0\x02\x00\x8B\x4E\x64\x8B\x11\x8B\x82\xAC\x00\x00\x00\x83\xC4\x04\xFF\xD0\xFF\x8E\x40\x76\x00\x00\x8B\x8E\x4C\x76\x00\x00\x8B\x89\x80\x00\x00\x00\x5E\xE9\x79\xEF\xF6\xFF";
const char* OnFrameStubMask = "xxxxxxxxxx?????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx??????";

DWORD OnFrameStubAddress = SignatureScanner::FindSignature(BaseAddress, DetourGetModuleSize((HMODULE)BASE_ADDRESS), OnFrameStubSignature, OnFrameStubMask);

class IDirect3D9;
class IDirect3DDevice9;

IDirect3DDevice9* Device;

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

HOOK(void, __fastcall, OnFrameStub, OnFrameStubAddress, void* This)
{
	RaiseEvents(modFrameEvents);
	CommonLoader::CommonLoader::RaiseUpdates();

	originalOnFrameStub(This);
}

void InitLoader()
{
	INSTALL_HOOK(SteamAPI_RestartAppIfNecessary);
	INSTALL_HOOK(SteamAPI_IsSteamRunning);
	INSTALL_HOOK(SteamAPI_Shutdown);
	INSTALL_HOOK(OnFrameStub);
	INSTALL_HOOK(ProcessStart);
}

void InitMods()
{
	ModsInfo = new ModInfo();
	ModsInfo->ModList = new vector<Mod*>();
	vector<ModInitEvent> postEvents;
	vector<ModInitEvent> initEvents;

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

		if(OnFrameStubAddress)
			InitLoader();
        
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

