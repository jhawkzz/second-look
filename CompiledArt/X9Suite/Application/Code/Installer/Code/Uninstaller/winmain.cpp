
#include "precompiled.h"
#include "winmain.h"
#include "IPage.h"

char g_ProgramPath[ MAX_PATH ];

DialogShell     g_DialogShell;
InstallSettings g_InstallSettings;

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
   //Watch for memory leaks in DEBUG only
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF );
	//End leak check

//   GetProgramPath( g_ProgramPath );

   // the uninstaller must copy itself to a temp folder and run from there,
   // so here we check to see if we are going to copy ourselves or run
#ifndef _DEBUG
      if ( !ParseCommandArgs( lpCmdLine ) )
      {
         return 0;
      }
#else
      #pragma message( "Building Debug - Uninstaller will not remove itself." )
#endif

   // create an event so we only run once.
   HANDLE runonceEvent = CreateEvent( NULL, FALSE, FALSE, IV_RUN_ONCE_MUTEX_STRING );
   DWORD error         = GetLastError( );

   CloseHandle( runonceEvent );

   if ( ERROR_ALREADY_EXISTS == error )
   {
      // exit
      char runOnceStr[ MAX_PATH ];
      sprintf( runOnceStr, "%s is currently running. Please close before uninstalling.", IV_PROGRAM_NAME );

      MessageBox( NULL, runOnceStr, "Setup", MB_OK | MB_ICONINFORMATION );
      return 0;
   }

   // begin program
   do
   {
      if ( !g_DialogShell.Create( ) )
      {
         MessageBox( NULL, "Failed to launch "PLUTO_NAME".", "Setup", MB_ICONERROR | MB_OK );
         break;
      }

      g_InstallSettings.Create( );

      MSG msg;
      while( 1 )
      {
         Timer::Update( );

         if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
         {  
            if ( WM_QUIT == msg.message )
            {
               break;
            }

            if ( !IsDialogMessage( g_DialogShell.GetCurrentPage( )->m_Hwnd, &msg ) )
            {
               TranslateMessage( &msg );
               DispatchMessage( &msg );
            }
         }
         Sleep( 0 );
      }

      g_InstallSettings.Destroy( );
      g_DialogShell.Destroy( );

      // if flagged for restart, handle that
      if ( g_InstallSettings.m_NeedsRestart )
      {
         UINT result = MessageBox( NULL, "You need to restart Windows to complete installation. Restart now?", IV_PROGRAM_NAME" Setup", MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON2 );

         if ( IDYES == result )
         {
            if ( !g_InstallSettings.RestartSystem( ) )
            {
               MessageBox( NULL, "Unable to automatically restart. Please restart to complete uninstallation.", IV_PROGRAM_NAME" Setup", MB_ICONINFORMATION | MB_OK );
            }
         }
      }
   }
   while( 0 );

   return 0;
}

void GetProgramPath( char * path )
{
  /* char *pCommandPath = GetCommandLine( );

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

   delete [] pFullPath;*/
}

int ParseCommandArgs( char *pStartArgs )
{
   // actually uninstall?
   if ( strstr( pStartArgs, "-uninstall" ) )
   {
      // just return 1, which means to proceed
      return 1;
   }

   // otherwise, we'll copy ourselves, launch ourselves, and shurt down
   char uninstallRegPath[ MAX_PATH ];
   sprintf( uninstallRegPath, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", IV_PROGRAM_NAME );

   // if we can't open the key, something horrible is happening.
   Registry_Class registry( HKEY_LOCAL_MACHINE );
   if ( !registry.Open_Key( uninstallRegPath, KEY_ALL_ACCESS, FALSE ) )
   {
      return 0;
   }

   char uninstallPath[ MAX_PATH ];
   registry.Get_Value_Data_String( "UninstallString", uninstallPath, MAX_PATH, "" );

   // copy the uninstaller to a temp folder
   char tempPath[ MAX_PATH ];
   GetTempPath( MAX_PATH, tempPath );

   // as I just learned thanks to NT 4, it's POSSIBLE that the temp folder doesn't exist, so try to make it
   CreateDirectory( tempPath, NULL );

   char fullTempPath[ MAX_PATH ];
   sprintf( fullTempPath, "%suninstall.exe", tempPath );

   CopyFile( uninstallPath, fullTempPath, FALSE );

   ShellExecute( NULL, "open", fullTempPath, "-uninstall", NULL, SW_SHOWNORMAL );

   return 0;
}

void WriteToLog( const char *pEntry )
{
   FILE *pFile = fopen( "C:\\errorLog.txt", "a" );

   char fullEntry[ MAX_PATH ];
   sprintf( fullEntry, "%s\r\n", pEntry );

   fwrite( fullEntry, 1, strlen( fullEntry ), pFile );

   fclose( pFile );
}
