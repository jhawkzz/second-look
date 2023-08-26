
#ifndef HOOK_API_H_
#define HOOK_API_H_

#include <windows.h>

int HookAPIFunction( HMODULE hModule, const char *pCurrentFunctionAddress, void *pCurrentFunction, void *pNewFunction );

#endif
