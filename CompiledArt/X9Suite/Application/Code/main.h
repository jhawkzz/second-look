
#ifndef MAIN_H_
#define MAIN_H_

#include "options.h"
#include "core.h"
#include "webserver.h"
#include "database.h"
#include "fileExplorer.h"
#include "log.h"
#include "commandBuffer.h"

#ifndef _DEBUG
#undef  _ASSERTE
#define _ASSERTE(a) if (!(a)) { g_Log.Write( Log::TypeError, "Error: %s line %d", __FUNCTION__, __LINE__ ); }
#endif

void CommandRun             ( void );
void GetProgramPath         ( char * path );
BOOL IsWindows64Bit         ( void );
BOOL GetSystemWow64Directory( LPTSTR lpBuffer, UINT uSize );

//globally define these so they can be accessed anywhere
extern Core           g_Core;
extern OptionsControl g_Options;
extern Webserver      g_Webserver;
extern Database       g_Database;
extern FileExplorer   g_FileExplorer;
extern Log            g_Log;
extern CommandBuffer  g_CommandBuffer;

#endif
