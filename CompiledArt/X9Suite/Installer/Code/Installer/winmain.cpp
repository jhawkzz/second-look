
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

   // create an event so we only run once.
   HANDLE runonceEvent = CreateEvent( NULL, FALSE, FALSE, IV_RUN_ONCE_MUTEX_STRING );
   DWORD error         = GetLastError( );

   CloseHandle( runonceEvent );

   if ( ERROR_ALREADY_EXISTS == error )
   {
      // exit
      char runOnceStr[ MAX_PATH ];
      sprintf( runOnceStr, "%s is currently running. Please close before installing.", IV_PROGRAM_NAME );

      MessageBox( NULL, runOnceStr, "Setup", MB_OK | MB_ICONINFORMATION );
      return 0;
   }

   if ( !g_InstallSettings.IsWindowsVersionSupported( ) )
   {
      MessageBox( NULL, "This operating system is not supported. Please run setup on a supported operating system.", "Setup", MB_OK | MB_ICONINFORMATION );
      return 0;
   }

   do
   {
      if ( !g_DialogShell.Create( ) )
      {
         MessageBox( NULL, "Failed to launch "PLUTO_NAME".", "Setup", MB_ICONERROR | MB_OK );
         break;
      }

      if ( !g_InstallSettings.Create( ) )
      {
         MessageBox( NULL, "Failed to launch "PLUTO_NAME".", "Setup", MB_ICONERROR | MB_OK );
         break;
      }

      HMODULE hLibrary = LoadLibrary( "RichEd20.dll" );
      if ( !hLibrary )
      {
         MessageBox( NULL, "Failed to load RichEd20.dll. Setup cannot run.", "Setup", MB_ICONERROR | MB_OK );
         break;
      }

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

      g_DialogShell.Destroy( );

      FreeLibrary( hLibrary );

      // if flagged for restart, handle that
      if ( g_InstallSettings.m_NeedsRestart )
      {
         UINT result = MessageBox( NULL, "You need to restart Windows to complete installation. Restart now?", IV_PROGRAM_NAME" Setup", MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON2 );

         if ( IDYES == result )
         {
            if ( !g_InstallSettings.RestartSystem( ) )
            {
               MessageBox( NULL, "Unable to automatically restart. Please restart to complete installation.", IV_PROGRAM_NAME" Setup", MB_ICONINFORMATION | MB_OK );
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

void WriteToLog( const char *pEntry )
{
   FILE *pFile = fopen( "C:\\errorLog.txt", "a" );

   char fullEntry[ MAX_PATH ];
   sprintf( fullEntry, "%s\r\n", pEntry );

   fwrite( fullEntry, 1, strlen( fullEntry ), pFile );

   fclose( pFile );
}
