#include "pch.h"
#include "Events.h"

std::vector<ModCallEvent> modFrameEvents;
std::vector<ModCallEvent> modExitEvents;
/**
* Registers an event to the specified event list.
* @param eventList The event list to add to.
* @param module The module for the mod DLL.
* @param name The name of the exported function from the module (i.e OnFrame)
*/
void RegisterEvent(std::vector<ModCallEvent>& eventList, HMODULE module, const char* name)
{
	ModCallEvent modEvent = (ModCallEvent)GetProcAddress(module, name);

	if (modEvent != nullptr)
		eventList.push_back(modEvent);
}
