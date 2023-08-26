
#include "hookAPI.h"
#include <psapi.h>
#include <shlwapi.h>

// define a function pointer for TerminateProcess
typedef BOOL (WINAPI *LPTerminateProcess)( HANDLE hProcess, UINT uExitCode );

//create a pointer we'll use to store the address of the terminate process we're replacing.
static LPTerminateProcess g_OriginalTerminateProcess = NULL;

// this is the prototype for our terminate process
BOOL WINAPI MyTerminateProcess( HANDLE hProcess, UINT uExitCode );

BOOL APIENTRY DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
   //TODO: Implement a proper setup for hooking any app that calls terminate process with X9 when it's opened
   //TODO: Test the hook chain by building an Inject2.dll/x9 that hooks after Inject.dll and outputs
   // a second message.
   //TODO: Determine why it isn't working on Vista
   //TODO: Make sure we don't check for x9 by name. use some ID in the program.
   
   //TODO: We probably want to find the function by name, then cross reference
   // that to the address, incase another program has already hooked it. That way we'll build
   // a chain. - DONE
   //TODO: Make our TerminateProcess properly check for X9 - DONE

   // Perform actions based on the reason for calling. 
   switch( fdwReason ) 
   {
      case DLL_PROCESS_ATTACH:
      {
         DWORD functionAddress = (DWORD) GetProcAddress( GetModuleHandle( "kernel32.dll" ), "TerminateProcess" );

         g_OriginalTerminateProcess = (LPTerminateProcess)functionAddress;

         int result = HookAPIFunction( GetModuleHandle( NULL ), "TerminateProcess", ( void *)functionAddress, &MyTerminateProcess );

         if ( result )
         {
            OutputDebugString( "Patched IAT!" );
         }
         else
         {
            OutputDebugString( "WARNING: FAILED TO PATCH IAT." );
         }

         break;
      }
      case DLL_PROCESS_DETACH:
      {
         // restore the original terminate process
         int result = HookAPIFunction( GetModuleHandle( NULL ), "TerminateProcess", &MyTerminateProcess, g_OriginalTerminateProcess );

         if ( result )
         {
            OutputDebugString( "Restored Original." );
         }
         else
         {
            OutputDebugString( "WARNING: FAILED TO RESTORE ORIGINAL" );
         }

         break;
      }
   }

   return TRUE;
}

BOOL WINAPI MyTerminateProcess( HANDLE hProcess, UINT uExitCode )
{
   // Task Manager only opened the process with TERMINATE_PROCESS rights. We need to dupe the handle and set more info so we can see if this is our app.
   // TODO: For Vista, we'll want to use: PROCESS_QUERY_LIMITED_INFORMATION
   HANDLE accessHandle;
   DuplicateHandle( GetCurrentProcess( ), hProcess, GetCurrentProcess( ), &accessHandle, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, 0 );

   BOOL denyAccess = FALSE;

   // get a list of all the modules used by this process
   char fileName[ MAX_PATH ];
   DWORD count = GetProcessImageFileName( accessHandle, fileName, MAX_PATH );
   if ( !count )
   {
      OutputDebugString( "\tFailed to get process image name\n" );
   }
   else
   {
      if ( StrStrI( fileName, "X9" ) )
      {
         denyAccess = TRUE;
      }
   }

   CloseHandle( accessHandle );

   if ( denyAccess )
   {
      SetLastError( ERROR_ACCESS_DENIED );
      return FALSE;
   }
   else
   {
      // since it's not X9, go ahead and allow it thru
      return g_OriginalTerminateProcess( hProcess, uExitCode );
   }
}
