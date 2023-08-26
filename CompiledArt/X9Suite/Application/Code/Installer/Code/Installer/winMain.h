
#ifndef WINMAIN_H_
#define WINMAIN_H_

#include "dialogShell.h"
#include "..\\Shared\\installSettings.h"

void GetProgramPath( char * path );
void WriteToLog( const char *pEntry );

extern DialogShell     g_DialogShell;
extern InstallSettings g_InstallSettings;

#endif
