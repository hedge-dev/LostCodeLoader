#pragma once

#include "LostCodeLoader.h"
#include <vector>

extern std::vector<ModCallEvent> modFrameEvents;
extern std::vector<ModCallEvent> modExitEvents;

/**
* Calls all registered events in the specified event list.
* @param eventList The list of events to trigger.
*/
inline void RaiseEvents(const std::vector<ModCallEvent>& eventList)
{
	for (auto& i : eventList)
		i();
}

/**
* Registers an event to the specified event list.
* @param eventList The event list to add to.
* @param module The module for the mod DLL.
* @param name The name of the exported function from the module (i.e OnFrame)
*/
void RegisterEvent(std::vector<ModCallEvent>& eventList, HMODULE module, const char* name);

