
#include "installSettings.h"
#include "registry_class.h"

//We need this so we can map the resources to file names under InstallMap
#include "..\\Installer\\resource.h" 

//SETUP INSTALLER HERE
#include <Setupapi.h>
#include <psapi.h>
#include <Aclapi.h>
#include "..\\..\\..\\AppIntegrityDriver\\appIntegrityDriverDefines.h"

// Put the file types that could be registered here
char g_FileTypeList[ ][ MAX_PATH ] = 
{
   ""
};

void InstallSettings::CreateInstallMaps( void )
{
   //SET UNINSTALL DETAILS
   strcpy( m_DisplayIconProgram, "Second Look.exe" ); // set the program whose icon you wish to display in the Add/Remove entry
   //END UNINSTALL DETAILS

   // add your mapped install files here.
   strcpy( m_pInstallFileMap[ IDR_RT_RCDATA1 ], "Second Look.exe"                       );
   strcpy( m_pInstallFileMap[ IDR_RT_RCDATA2 ], DRIVER_NAME_NON_UNICODE"_x86.inf"       );
   strcpy( m_pInstallFileMap[ IDR_RT_RCDATA3 ], "uninstall.exe"                         );
   strcpy( m_pInstallFileMap[ IDR_RT_RCDATA4 ], DRIVER_NAME_NON_UNICODE"_XP.sys"        );
   strcpy( m_pInstallFileMap[ IDR_RT_RCDATA5 ], DRIVER_NAME_NON_UNICODE"_Vista_x86.sys" );
   strcpy( m_pInstallFileMap[ IDR_RT_RCDATA6 ], DRIVER_NAME_NON_UNICODE"_Vista_x64.sys" );
   strcpy( m_pInstallFileMap[ IDR_RT_RCDATA7 ], DRIVER_NAME_NON_UNICODE"_Win7_x64.sys"  );
   strcpy( m_pInstallFileMap[ IDR_RT_RCDATA8 ], DRIVER_NAME_NON_UNICODE"_x64.inf"       );

   // add your mapped dll files here

   // add your required install packages here

   // add your shortcuts here
   strcpy( m_pShortcutMap[ 0 ].shortcutName  , "Second Look"     );
   strcpy( m_pShortcutMap[ 0 ].shortcutTarget, "Second Look.exe" );
   strcpy( m_pShortcutMap[ 0 ].shortcutSubDir, "Second Look"     );
   m_pShortcutMap[ 0 ].shortcutType = CSIDL_PROGRAMS;

   strcpy( m_pShortcutMap[ 1 ].shortcutName  , "Second Look"     );
   strcpy( m_pShortcutMap[ 1 ].shortcutTarget, "Second Look.exe" );
   m_pShortcutMap[ 1 ].shortcutType = CSIDL_DESKTOPDIRECTORY;

   memset( m_RegistryValuesByInstaller, 0, sizeof( m_RegistryValuesByInstaller ) );
   memset( m_RegistryKeysToRemove     , 0, sizeof( m_RegistryKeysToRemove      ) );
   memset( m_RegistryValuesToRemove   , 0, sizeof( m_RegistryValuesToRemove    ) );
   memset( m_FileTypeAssociationValue , 0, sizeof( m_FileTypeAssociationValue  ) );
   memset( m_ExtraFilesToRemove       , 0, sizeof( m_ExtraFilesToRemove        ) );

   // add registry values here that the installer should add/remove
   m_RegistryValuesByInstaller[ 0 ].rootKey = HKEY_LOCAL_MACHINE;
   m_RegistryValuesByInstaller[ 0 ].type    = REG_VALUE_TYPE_STRING;
   strcpy( m_RegistryValuesByInstaller[ 0 ].key  , "Software\\Microsoft\\Windows\\CurrentVersion\\Run" );
   strcpy( m_RegistryValuesByInstaller[ 0 ].value, "Second Look" );
   strcpy( m_RegistryValuesByInstaller[ 0 ].data , REG_INSTALL_APP_MACRO );

   // add registry keys that your app will create, which need to be removed, here

   // add registry values that your app will create, which need to be removed, here

   // put the values used to claim file associations here

   // put the backup value for file type restoration

   // put files your program will create, that need to be cleaned up, right here
   char appDataDir[ MAX_PATH ];
   SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataDir );
   
   sprintf( m_ExtraFilesToRemove[ 0 ], "%s\\Second Look\\Second Look.pak", appDataDir );
   sprintf( m_ExtraFilesToRemove[ 1 ], "%s\\Second Look\\settings.dat"   , appDataDir );
   
   char systemDriversDir[ MAX_PATH ];
   SHGetFolderPath( NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, systemDriversDir );
   sprintf( m_ExtraFilesToRemove[ 2 ], "%s\\drivers\\%s.sys", systemDriversDir, DRIVER_NAME_NON_UNICODE );
}

BOOL InstallSettings::AppSpecificInstall( void )
{
   // if you need to do custom installation stuff after files are installed, do it here.

   // install our driver
   BOOL success = FALSE;
   HINF hInf    = INVALID_HANDLE_VALUE;

   do
   {
      // ---- determine the version of windows we're running and setup the correct driver ----
      char osDriverName[ MAX_PATH ];

      char driverName[ MAX_PATH ];
      sprintf( driverName, "%s\\%s.sys", m_InstallDir, DRIVER_NAME_NON_UNICODE );

      // for Vista SP0, we can actually just use our XP driver. 
      // Both will be 32bit, we don't support 64 bit XP or VistaSP0.
      if ( IsWindowsXP( ) || IsWindowsVistaSP0( ) )
      {
         sprintf( osDriverName, "%s\\%s_XP.sys", m_InstallDir, DRIVER_NAME_NON_UNICODE );
      }

      if ( IsWindowsVistaSP1Plus( ) )
      {
         if ( IsWindows64Bit( ) )
         {
            sprintf( osDriverName, "%s\\%s_Vista_x64.sys", m_InstallDir, DRIVER_NAME_NON_UNICODE );
         }
         else
         {
            sprintf( osDriverName, "%s\\%s_Vista_x86.sys", m_InstallDir, DRIVER_NAME_NON_UNICODE );
         }
      }

      if ( IsWindows7( ) )
      {
         if ( IsWindows64Bit( ) )
         {
            sprintf( osDriverName, "%s\\%s_Win7_x64.sys", m_InstallDir, DRIVER_NAME_NON_UNICODE );
         }
         else
         {
            // we can just use the vista x86 driver if it's Win7 32bit.
            sprintf( osDriverName, "%s\\%s_Vista_x86.sys", m_InstallDir, DRIVER_NAME_NON_UNICODE );
         }
      }

      // rename the os driver to the official name, since we want that one.
      MoveFile( osDriverName, driverName );

      // now delete all of the 'os specific' drivers
      sprintf( osDriverName, "%s\\%s_XP.sys", m_InstallDir, DRIVER_NAME_NON_UNICODE );
      DeleteFile( osDriverName );

      sprintf( osDriverName, "%s\\%s_Vista_x86.sys", m_InstallDir, DRIVER_NAME_NON_UNICODE );
      DeleteFile( osDriverName );

      sprintf( osDriverName, "%s\\%s_Vista_x64.sys", m_InstallDir, DRIVER_NAME_NON_UNICODE );
      DeleteFile( osDriverName );

      sprintf( osDriverName, "%s\\%s_Win7_x64.sys", m_InstallDir, DRIVER_NAME_NON_UNICODE );
      DeleteFile( osDriverName );

      // ---- determine the correct INF to use ----
      char osInfName[ MAX_PATH ];

      char infName[ MAX_PATH ];
      sprintf( infName, "%s\\%s.inf", m_InstallDir, DRIVER_NAME_NON_UNICODE );

      if ( IsWindows64Bit( ) )
      {
         sprintf( osInfName, "%s\\%s_x64.inf", m_InstallDir, DRIVER_NAME_NON_UNICODE );
      }
      else
      {
         sprintf( osInfName, "%s\\%s_x86.inf", m_InstallDir, DRIVER_NAME_NON_UNICODE );
      }

      // rename the os inf to the official name
      MoveFile( osInfName, infName );

      // now delete all the 'os specific' infs
      sprintf( osInfName, "%s\\%s_x64.inf", m_InstallDir, DRIVER_NAME_NON_UNICODE );
      DeleteFile( osInfName );

      sprintf( osInfName, "%s\\%s_x86.inf", m_InstallDir, DRIVER_NAME_NON_UNICODE );
      DeleteFile( osInfName );


      // ---- create a command line for launching the INF installer silently ----
      char commandLine[ MAX_PATH ];
      sprintf( commandLine, "advpack.dll,LaunchINFSection %s\\%s.inf,DefaultInstall,3", m_InstallDir, DRIVER_NAME_NON_UNICODE );
      
      // ---- launch the installer ----
      SHELLEXECUTEINFO shellExecuteInfo = { 0 };
      shellExecuteInfo.cbSize       = sizeof( shellExecuteInfo );
      shellExecuteInfo.fMask        = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
      shellExecuteInfo.lpVerb       = "open";
      shellExecuteInfo.lpFile       = "RUNDLL32.EXE";
      shellExecuteInfo.lpParameters = commandLine;
      shellExecuteInfo.nShow        = SW_HIDE;

      if ( !ShellExecuteEx( &shellExecuteInfo ) ) break;

      // wait for the process we launched to finish
      DWORD exitCode = STILL_ACTIVE;
      while( STILL_ACTIVE == exitCode )
      {
         GetExitCodeProcess ( shellExecuteInfo.hProcess, &exitCode );
      }

      // ---- install the service ----
      char infFilePath[ MAX_PATH ];
      sprintf( infFilePath, "%s\\%s.inf", m_InstallDir, DRIVER_NAME_NON_UNICODE );
      hInf = SetupOpenInfFile( infFilePath, NULL, INF_STYLE_WIN4, NULL );

      SetupInstallServicesFromInfSection( hInf, "DefaultInstall.Services", 0 );

      // ---- now remove the driver from the install directory ----
      char driverPath[ MAX_PATH ];
      sprintf( driverPath, "%s\\%s.sys", m_InstallDir, DRIVER_NAME_NON_UNICODE );

      DeleteFile( driverPath );

      // ---- now remove the inf from the install directory ----
      char infPath[ MAX_PATH ];
      sprintf( infPath, "%s\\%s.inf", m_InstallDir, DRIVER_NAME_NON_UNICODE );

      DeleteFile( infPath );

      success = TRUE;
   }
   while( 0 );

   if ( hInf != INVALID_HANDLE_VALUE )
   {
      SetupCloseInfFile( hInf );
   }

   m_NeedsRestart = TRUE;

   return success;
}

void InstallSettings::AppSpecificUninstall( void )
{
   BOOL success        = FALSE;
   BOOL driverStopped  = FALSE;
   HANDLE hToken       = NULL;
   char *pTokenUser    = NULL;
   SC_HANDLE scmHandle = NULL;
   SC_HANDLE schDriver = NULL;
   ACL *pNewDacl       = NULL;
   char *pPrevPriv     = NULL;

   do
   {
       // ---- create a command line for launching the INF uninstaller silently ----
      //char commandLine[ MAX_PATH ];
      //sprintf( commandLine, "advpack.dll,LaunchINFSection %s\\%s.inf,DefaultUnInstall,3", m_InstallDir, DRIVER_NAME_NON_UNICODE );
      //
      //// ---- launch the uninstaller ----
      //SHELLEXECUTEINFO shellExecuteInfo = { 0 };
      //shellExecuteInfo.cbSize       = sizeof( shellExecuteInfo );
      //shellExecuteInfo.fMask        = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
      //shellExecuteInfo.lpVerb       = "open";
      //shellExecuteInfo.lpFile       = "RUNDLL32.EXE";
      //shellExecuteInfo.lpParameters = commandLine;
      //shellExecuteInfo.nShow        = SW_HIDE;

      //if ( !ShellExecuteEx( &shellExecuteInfo ) ) break;

      //// wait for the process we launched to finish
      //DWORD exitCode = STILL_ACTIVE;
      //while( STILL_ACTIVE == exitCode )
      //{
      //   GetExitCodeProcess ( shellExecuteInfo.hProcess, &exitCode );
      //}


      // ---- adjust token privileges and lookup SID----
      // get the token for this process
      if ( !OpenProcessToken( GetCurrentProcess( ), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken ) ) break;

      // find the ownership privilege
      LUID luid;
      if ( !LookupPrivilegeValue(0, SE_TAKE_OWNERSHIP_NAME, &luid) ) break;

      // adjust our privileges so ownership is enabled
      TOKEN_PRIVILEGES priv;
      priv.PrivilegeCount             = 1;
      priv.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;
      priv.Privileges[ 0 ].Luid       = luid;

      // get the current privilege info so we can restore it. We call twice to first get the buffer size, then the info
      DWORD prevPrivSize;
      AdjustTokenPrivileges( hToken, TokenPrivileges, &priv, 1, (TOKEN_PRIVILEGES *)&prevPrivSize, &prevPrivSize );
      
      DWORD error = GetLastError( );
      if ( error != ERROR_INSUFFICIENT_BUFFER ) break;

      // // adjust our privileges so ownership is enabled, and get the old state
      pPrevPriv = new char[ prevPrivSize ];
      if ( !AdjustTokenPrivileges( hToken, FALSE, &priv, prevPrivSize, (TOKEN_PRIVILEGES *)pPrevPriv, &prevPrivSize ) ) break;

      // get the SID for our token. We call twice to first get the buffer size, then to get the info
      DWORD tokenInfoSize;
      GetTokenInformation( hToken, TokenUser, NULL, 0, &tokenInfoSize );

      error = GetLastError( );
      if ( error != ERROR_INSUFFICIENT_BUFFER ) break;
      
      // get our SID
      pTokenUser = new char[ tokenInfoSize ];
      if ( !GetTokenInformation( hToken, TokenUser, pTokenUser, tokenInfoSize, &tokenInfoSize ) ) break;


      // ---- modify driver ----
      //We need to give ourselves permission to delete the driver.
      // this is a 3 step process. 
      //1. Open to set the owner, and make it us.
      //2. Open to set the DACL, and allow us permission to delete
      //3. Open to delete and delete.

      // now open the service manager
      scmHandle = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
      if ( !scmHandle ) break;
   
      // ---- take ownership of driver ----
      schDriver = OpenServiceW( scmHandle, DRIVER_NAME, WRITE_OWNER );
      if ( !schDriver ) break;

      //prepare a security descriptor for our service, with us as the owner
      SECURITY_DESCRIPTOR securityDescriptor;
      if ( !InitializeSecurityDescriptor( &securityDescriptor, SECURITY_DESCRIPTOR_REVISION ) ) break;

      // set our SID in the service security descriptor
      if ( !SetSecurityDescriptorOwner( &securityDescriptor, ((TOKEN_USER *)pTokenUser)->User.Sid, FALSE ) ) break;

      // apply this SID to the service, making us an owner
      if ( !SetServiceObjectSecurity( schDriver, OWNER_SECURITY_INFORMATION, &securityDescriptor ) ) break;

      // close the service, we are now an owner.
      CloseServiceHandle( schDriver );


      // ---- set the DACL to give us permission to delete driver ----
      // open the driver for writing the DACL
      schDriver = OpenServiceW( scmHandle, DRIVER_NAME, WRITE_DAC );
      if ( !schDriver ) break;

      // create an ACE taht gives everyone (that's us!) permission to stop and delete
      EXPLICIT_ACCESS explicitAccess;
      BuildExplicitAccessWithName( &explicitAccess, TEXT("EVERYONE"), GENERIC_READ | SERVICE_START | SERVICE_STOP | READ_CONTROL | DELETE, SET_ACCESS, NO_INHERITANCE );

      // create a dacl
      if ( SetEntriesInAcl( 1, &explicitAccess, NULL, &pNewDacl ) != ERROR_SUCCESS ) break;

      // re-init the security descriptor
      if ( !InitializeSecurityDescriptor( &securityDescriptor, SECURITY_DESCRIPTOR_REVISION ) ) break;

      // set the dacl in this descriptor
      if ( !SetSecurityDescriptorDacl( &securityDescriptor, TRUE, pNewDacl, FALSE ) ) break;

      // and set the dacl for the service. Cross your fingers!
      if ( !SetServiceObjectSecurity( schDriver, DACL_SECURITY_INFORMATION, &securityDescriptor ) ) break;

      //FINALLY, we should be able to close/open this driver and delete it.
      CloseServiceHandle( schDriver );

      
      // ---- delete the service ----
      schDriver = OpenServiceW( scmHandle, DRIVER_NAME, DELETE );
      if ( !schDriver )
      {
         break;
      }

      if ( !DeleteService( schDriver ) )
      {
         DWORD lastError = GetLastError( );

         // if we failed and it WASN'T marked for delete, that's bad
         if ( lastError != ERROR_SERVICE_MARKED_FOR_DELETE )
         {
            break;
         }
      }

      success = TRUE;
   }
   while( 0 );
   
   if ( pNewDacl )
   {
      LocalFree( pNewDacl );
   }

   if ( schDriver )
   {
      CloseServiceHandle( schDriver );
   }

   if ( scmHandle )
   {
      CloseServiceHandle( scmHandle );
   }

   if ( pTokenUser )
   {
      delete [] pTokenUser;
   }

   if ( pPrevPriv )
   {
      // restore previous privileges before deleting
      AdjustTokenPrivileges( hToken, FALSE, (TOKEN_PRIVILEGES *)pPrevPriv, 0, NULL, 0 );
      delete [] pPrevPriv;
   }

   if ( hToken )
   {
      CloseHandle( hToken );
   }

   m_NeedsRestart = TRUE;
}
//END SETUP INSTALLER

InstallSettings::InstallSettings( void )
{
   m_EulaAgreed            = FALSE;
   m_InstallDesktopIcons   = TRUE;
   m_InstallStartMenuItems = TRUE;
   m_IsAppAlreadyInstalled = FALSE;
   m_NeedsRestart          = FALSE;
   m_InstallDir[ 0 ]       = 0;

   // allocate memory for the install and DLL registration maps.
   m_pInstallFileMap      = (char **) new char *[ MAX_MAP_ENTRIES ];
   m_pDLLRegisterMap      = (char **) new char *[ MAX_MAP_ENTRIES ];
   m_pInstallPackageMap   = (char **) new char *[ MAX_MAP_ENTRIES ];
   m_pInstallPackageIDMap = (char **) new char *[ MAX_MAP_ENTRIES ];
   
   m_pInstallFileMapData      = new char[ MAX_PATH * MAX_MAP_ENTRIES ];
   m_pDLLRegisterMapData      = new char[ MAX_PATH * MAX_MAP_ENTRIES ];
   m_pInstallPackageMapData   = new char[ MAX_PATH * MAX_MAP_ENTRIES ];
   m_pInstallPackageIDMapData = new char[ MAX_PATH * MAX_MAP_ENTRIES ];

   memset( m_pInstallFileMapData     , 0, MAX_PATH * MAX_MAP_ENTRIES );
   memset( m_pDLLRegisterMapData     , 0, MAX_PATH * MAX_MAP_ENTRIES );
   memset( m_pInstallPackageMapData  , 0, MAX_PATH * MAX_MAP_ENTRIES );
   memset( m_pInstallPackageIDMapData, 0, MAX_PATH * MAX_MAP_ENTRIES );
   
   char *pCurrFileMapPos      = m_pInstallFileMapData;
   char *pCurrDLLMapPos       = m_pDLLRegisterMapData;
   char *pCurrPackageMapPos   = m_pInstallPackageMapData;
   char *pCurrPackageIDMapPos = m_pInstallPackageIDMapData;

   uint32 i;
   for ( i = 0; i < MAX_MAP_ENTRIES; i++ )
   {
      m_pInstallFileMap[ i ]      = pCurrFileMapPos;
      m_pDLLRegisterMap[ i ]      = pCurrDLLMapPos;
      m_pInstallPackageMap[ i ]   = pCurrPackageMapPos;
      m_pInstallPackageIDMap[ i ] = pCurrPackageIDMapPos;
      
      pCurrFileMapPos      += MAX_PATH;
      pCurrDLLMapPos       += MAX_PATH;
      pCurrPackageMapPos   += MAX_PATH;
      pCurrPackageIDMapPos += MAX_PATH;
   }

   m_pShortcutMap = new ShortcutObject[ MAX_MAP_ENTRIES ];
   memset( m_pShortcutMap, 0, sizeof( ShortcutObject ) * MAX_MAP_ENTRIES );
   
   CreateInstallMaps( );
}

InstallSettings::~InstallSettings( )
{
   delete [] m_pInstallFileMapData;
   delete [] m_pDLLRegisterMapData;
   delete [] m_pInstallPackageMapData;
   delete [] m_pInstallPackageIDMapData;

   delete [] m_pInstallFileMap;
   delete [] m_pDLLRegisterMap;
   delete [] m_pInstallPackageMap;
   delete [] m_pInstallPackageIDMap;

   delete [] m_pShortcutMap;

   Destroy( );
}

BOOL InstallSettings::Create( void )
{
   sprintf( m_UninstallRegPath, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", IV_UNINSTALL_REG_KEY );

   // get the default install path.
   Registry_Class registry( HKEY_LOCAL_MACHINE );

   // but let's see if they HAVE installed it, and if so, suggest that path.
   if ( registry.Open_Key( m_UninstallRegPath, KEY_ALL_ACCESS, FALSE ) )
   {
      m_IsAppAlreadyInstalled = TRUE;
      m_InstallDesktopIcons   = registry.Get_Value_Data_Int( "InstallDesktopIcons"  , 1 );
      m_InstallStartMenuItems = registry.Get_Value_Data_Int( "InstallStartMenuItems", 1 );
   }

   GetDefaultInstallDir( );

   return TRUE;
}

void InstallSettings::Destroy( void )
{
}

void InstallSettings::GetDefaultInstallDir( void )
{
   // get the default install path.
   Registry_Class registry( HKEY_LOCAL_MACHINE );
   BOOL alreadyInstalled = FALSE; //assume it is NOT installed

   // but let's see if they HAVE installed it, and if so, suggest that path.
   if ( registry.Open_Key( m_UninstallRegPath, KEY_ALL_ACCESS, FALSE ) )
   {
      // if this returns false, then alreadyInstalled remains FALSE, and we'll create a default one below.
      alreadyInstalled = registry.Get_Value_Data_String( "InstallDir", m_InstallDir, MAX_PATH - 1, "" );
   }

   // not installed (or missing the InstallPath reg), so let's suggest their default program files folder.
   if ( !alreadyInstalled )
   {
      char programFilesDir[ MAX_PATH ];

      // get their default program files path.
      SHGetFolderPath( NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, programFilesDir );

      // append as much of the install folder as we can
      uint32 programDirLen = strlen( programFilesDir );
      uint32 availableLen  = MAX_PATH - programDirLen;

      _snprintf( m_InstallDir, MAX_PATH - 1, "%s\\%s", programFilesDir, IV_DEFAULT_PROGRAM_FOLDER );
   }
}

BOOL InstallSettings::RestartSystem( void )
{
   BOOL success        = FALSE;
   HANDLE hToken       = NULL;
   char *pPrevPriv     = NULL;

   // ---- adjust token privileges ----
   do
   {
      // get the token for this process
      if ( !OpenProcessToken( GetCurrentProcess( ), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken ) ) break;

      // find the ownership privilege
      LUID luid;
      if ( !LookupPrivilegeValue(0, SE_SHUTDOWN_NAME, &luid) ) break;

      // adjust our privileges so ownership is enabled
      TOKEN_PRIVILEGES priv;
      priv.PrivilegeCount             = 1;
      priv.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;
      priv.Privileges[ 0 ].Luid       = luid;

      // get the current privilege info so we can restore it. We call twice to first get the buffer size, then the info
      DWORD prevPrivSize;
      AdjustTokenPrivileges( hToken, TokenPrivileges, &priv, 1, (TOKEN_PRIVILEGES *)&prevPrivSize, &prevPrivSize );
      
      DWORD error = GetLastError( );
      if ( error != ERROR_INSUFFICIENT_BUFFER ) break;

      // // adjust our privileges so ownership is enabled, and get the old state
      pPrevPriv = new char[ prevPrivSize ];
      if ( !AdjustTokenPrivileges( hToken, FALSE, &priv, prevPrivSize, (TOKEN_PRIVILEGES *)pPrevPriv, &prevPrivSize ) ) break;

      // ---- perform system restart ----
      InitiateSystemShutdown( NULL, NULL, 0, FALSE, TRUE );

      success = TRUE;
   }
   while( 0 );

   if ( pPrevPriv )
   {
      // restore previous privileges before deleting
      AdjustTokenPrivileges( hToken, FALSE, (TOKEN_PRIVILEGES *)pPrevPriv, 0, NULL, 0 );
      delete [] pPrevPriv;
   }

   return success;
}

BOOL InstallSettings::IsWindowsVersionSupported( void )
{
   BOOL operatingSystemSupported = FALSE;

   do
   {
      // ---- get the windows version info ----
      OSVERSIONINFOEX osVersionInfoEx = { 0 };
      osVersionInfoEx.dwOSVersionInfoSize = sizeof( osVersionInfoEx );

      GetVersionEx( (OSVERSIONINFO *)&osVersionInfoEx );

      /*// Display the OS version info to the user, for debugging
      char sixtyFourBitStr[ MAX_PATH ];
      sprintf( sixtyFourBitStr, "64Bit: %s", IsWindows64Bit( ) ? "Yes" : "No" );

      char versionInfo[ MAX_PATH ];
      sprintf( versionInfo, "dwMajor: %d\n"
                             "dwMinor: %d\n"
                             "dwBuildNumber: %d\n"
                             "dwPlatformId: %d\n"
                             "wServicePackMajor: %d\n"
                             "wServicePackMinor: %d\n"
                             "%s"                     , osVersionInfoEx.dwMajorVersion, 
                                                        osVersionInfoEx.dwMinorVersion, 
                                                        osVersionInfoEx.dwBuildNumber, 
                                                        osVersionInfoEx.dwPlatformId, 
                                                        osVersionInfoEx.wServicePackMajor, 
                                                        osVersionInfoEx.wServicePackMinor,
                                                        sixtyFourBitStr );

      MessageBox( NULL, versionInfo, "Version", MB_OK );*/

      // we don't support NT/9X/ME
      if ( WIN_2K_XP_2K3_XP64_HOMESERVER_MAJOR > osVersionInfoEx.dwMajorVersion ) break;

      // we don't support any server versions (Home Server, 2K Server, 2K3, 2K8)
      if ( osVersionInfoEx.wProductType != VER_NT_WORKSTATION ) break;

      // we also don't support Windows 2K... (if we get here we're the lowest we can be is 2K)
      if ( WIN_2K_MAJOR == osVersionInfoEx.dwMajorVersion && WIN_2K_MINOR == osVersionInfoEx.dwMinorVersion ) break;

      // we specifically will not support Windows XP 64 bit
      if ( IsWindowsXP( ) && IsWindows64Bit( ) ) break;

      // we also specifically will not support Vista SP0 64bit.
      if ( IsWindowsVistaSP0( ) && IsWindows64Bit( ) ) break;

      operatingSystemSupported = TRUE;
   }
   while( 0 );

   return operatingSystemSupported;
}

BOOL InstallSettings::IsWindowsXP( void )
{
   // ---- get the windows version info ----
   OSVERSIONINFOEX osVersionInfoEx = { 0 };
   osVersionInfoEx.dwOSVersionInfoSize = sizeof( osVersionInfoEx );

   GetVersionEx( (OSVERSIONINFO *)&osVersionInfoEx );

   if ( 5 == osVersionInfoEx.dwMajorVersion && 1 == osVersionInfoEx.dwMinorVersion )
   {
      return TRUE;
   }

   return FALSE;
}

BOOL InstallSettings::IsWindowsVistaSP0( void )
{
   // ---- get the windows version info ----
   OSVERSIONINFOEX osVersionInfoEx = { 0 };
   osVersionInfoEx.dwOSVersionInfoSize = sizeof( osVersionInfoEx );

   GetVersionEx( (OSVERSIONINFO *)&osVersionInfoEx );

   if ( 6 == osVersionInfoEx.dwMajorVersion && 0 == osVersionInfoEx.dwMinorVersion && 0 == osVersionInfoEx.wServicePackMajor )
   {
      return TRUE;
   }

   return FALSE;
}

BOOL InstallSettings::IsWindowsVistaSP1Plus( void )
{
   // ---- get the windows version info ----
   OSVERSIONINFOEX osVersionInfoEx = { 0 };
   osVersionInfoEx.dwOSVersionInfoSize = sizeof( osVersionInfoEx );

   GetVersionEx( (OSVERSIONINFO *)&osVersionInfoEx );

   if ( 6 == osVersionInfoEx.dwMajorVersion && 0 == osVersionInfoEx.dwMinorVersion && osVersionInfoEx.wServicePackMajor > 0 )
   {
      return TRUE;
   }

   return FALSE;
}

BOOL InstallSettings::IsWindows7( void )
{
   // ---- get the windows version info ----
   OSVERSIONINFOEX osVersionInfoEx = { 0 };
   osVersionInfoEx.dwOSVersionInfoSize = sizeof( osVersionInfoEx );

   GetVersionEx( (OSVERSIONINFO *)&osVersionInfoEx );

   if ( 6 == osVersionInfoEx.dwMajorVersion && 1 == osVersionInfoEx.dwMinorVersion )
   {
      return TRUE;
   }

   return FALSE;
}

BOOL InstallSettings::IsWindows64Bit( void )
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
