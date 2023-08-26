
#include "precompiled.h"
#include "main.h"
#include "serviceInit.h"

Core           g_Core;
OptionsControl g_Options;
Webserver      g_Webserver;
Database       g_Database;
FileExplorer   g_FileExplorer;
Log            g_Log;
CommandBuffer  g_CommandBuffer;

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
   #ifdef INSTALL_SERVICE
      ServiceInit::ServiceInstall( );
      return 0;
   #endif

   #ifdef UNINSTALL_SERVICE
      ServiceInit::ServiceUnInstall( );
      return 0;
   #endif
   
   #ifdef RUN_AS_SERVICE
      ServiceInit::ServiceRun( ); //this will setup all service calls and run in a service thread
   #else
      CommandRun( ); //this will setup all calls and run as a command app.
   #endif

   return 0;
}

void CommandRun( void )
{
   srand( GetTickCount( ) );

   // running as a command app is pretty. darn. simple.
   if ( !g_Core.Initialize( 0, NULL ) )
   {
      g_Core.UnInitialize( );
      return;
   }
   
   g_Core.Tick( );

   g_Core.UnInitialize( );
}

void GetProgramPath( char * path )
{
   char *pCommandPath = GetCommandLine( );

   uint32 pathSize = (uint32) strlen( pCommandPath ) + 1;
   char *pFullPath = new char[ pathSize ];
   
   memset( pFullPath, 0, pathSize );

	strcpy( pFullPath, pCommandPath );
	char * fp = pFullPath;

	while ( *( ++fp ) != '\"' ) { };
	
   *( ++fp ) = 0;

	fp = strrchr ( pFullPath, '\\' );

	*( ++fp ) = 0;

	if ( pFullPath[ 0 ] == 34 )
   {
		strcpy ( path, pFullPath + 1 );
   }
	else
   {
		strcpy ( path, pFullPath );
   }

   delete [] pFullPath;
}

BOOL IsWindows64Bit( void )
{
   BOOL is64BitOS = FALSE;

   // We check if the OS is 64 Bit
   typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);

   LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress( GetModuleHandle("kernel32"),"IsWow64Process" );

   if (NULL != fnIsWow64Process)
   {
      fnIsWow64Process( GetCurrentProcess(), &is64BitOS );
   }

   return is64BitOS;
}

// Get the WOW64 folder. (We have to import the function address)
BOOL GetSystemWow64Directory( LPTSTR lpBuffer, UINT uSize )
{
   typedef UINT (WINAPI *LPFN_GETSYSWOW64DIR)(LPTSTR, UINT);

   LPFN_GETSYSWOW64DIR fnGetSystemWow64Dir = (LPFN_GETSYSWOW64DIR)GetProcAddress( GetModuleHandle("kernel32"),"GetSystemWow64DirectoryA" );

   if ( NULL != fnGetSystemWow64Dir )
   {
      fnGetSystemWow64Dir( lpBuffer, uSize );
      return TRUE;
   }

   return FALSE;
}
