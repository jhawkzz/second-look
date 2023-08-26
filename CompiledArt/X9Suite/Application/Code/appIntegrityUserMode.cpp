
#include "precompiled.h"
#include "main.h"
#include "appIntegrityUserMode.h"
#include "serviceInit.h"
#include "registry_class.h"
#include "..\\..\\AppIntegrityDriver\\appIntegrityDriverDefines.h"

#define DRIVER_USER_MODE_EVENT "AppIntegrityUserModeEvent"

AppIntegrityUserMode::AppIntegrityUserMode( void )
{
   m_hDriver = INVALID_HANDLE_VALUE;
}

AppIntegrityUserMode::~AppIntegrityUserMode( )
{
}

BOOL AppIntegrityUserMode::EnableIntegrity( void )
{
   OutputDebugString( "Enable Integrity\n" );

   BOOL success = FALSE;

   do
   {
      // ---- now obtain a real handle to the driver ----
      m_hDriver = CreateFileW( L"\\\\.\\"DRIVER_NAME, 
                               GENERIC_READ | GENERIC_WRITE, 
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
			                      0,
			                      OPEN_EXISTING,
			                      0,
			                      0 );

      if ( INVALID_HANDLE_VALUE == m_hDriver )
      {
         OutputDebugString( "\tCould not open driver handle. Is driver installed and running?\n" );
         break;
      }

      // send the process ID of the this process, which is what we want protected.
      ULONG_PTR processId = (ULONG_PTR)GetProcessId( GetCurrentProcess( ) );
      char outputStr[ MAX_PATH ];
      sprintf( outputStr, "\tProcess ID: %d\n", processId ); 
      OutputDebugString( outputStr );

      DWORD bytesReturned;
      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROCESS_ID, &processId, sizeof( processId ), NULL, 0, &bytesReturned, NULL );


      // ---- Set which Registry Keys to protect ----
      PROTECTED_REG_ITEM_INFO protectedItem = { 0 };

      // all our keys and values reside in HKEY_LOCAL_MACHINE
      protectedItem.rootClass = (ULONG)HandleToUlong(HKEY_LOCAL_MACHINE);

      // Set Values
      protectedItem.isValue = TRUE;

      // protect the x9 software startup reg value
      if ( IsWindows64Bit( ) )
      {
         // protect the x64 software startup reg value
         wcscpy( protectedItem.keyPath, L"Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run" );
         MultiByteToWideChar( CP_ACP, 0, OptionsControl::RegistryRunValue, -1, protectedItem.valueName, MAX_PATH );
         DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROTECTED_REG_ITEM, &protectedItem, sizeof( protectedItem ), NULL, 0, &bytesReturned, NULL );
      }
      else
      {
         wcscpy( protectedItem.keyPath, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run" );
         MultiByteToWideChar( CP_ACP, 0, OptionsControl::RegistryRunValue, -1, protectedItem.valueName, MAX_PATH );
         DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROTECTED_REG_ITEM, &protectedItem, sizeof( protectedItem ), NULL, 0, &bytesReturned, NULL );
      }


      //Set Keys
      protectedItem.isValue        = FALSE;
      protectedItem.valueName[ 0 ] = 0;

      // protect the run key itself. Again protect the appropriate key based on which platform we're on.
      if ( IsWindows64Bit( ) )
      {
         wcscpy( protectedItem.keyPath, L"Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run" );
         DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROTECTED_REG_ITEM, &protectedItem, sizeof( protectedItem ), NULL, 0, &bytesReturned, NULL );
      }
      else
      {
         wcscpy( protectedItem.keyPath, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run" );
         DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROTECTED_REG_ITEM, &protectedItem, sizeof( protectedItem ), NULL, 0, &bytesReturned, NULL );
      }

      // protect the X9 options
      protectedItem.protectAllValues = TRUE;
      MultiByteToWideChar( CP_ACP, 0, OptionsControl::OptionsRegKey, -1, protectedItem.keyPath, MAX_PATH );
      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROTECTED_REG_ITEM, &protectedItem, sizeof( protectedItem ), NULL, 0, &bytesReturned, NULL );

      // Protect the AppIntegrityDriver's Installation Reg Key
      // first, find out which Registry ControlSet is active
      Registry_Class registry( HKEY_LOCAL_MACHINE );
      registry.Open_Key( "System\\Select", KEY_READ );
      DWORD controlSetIndex = registry.Get_Value_Data_Int( "Current", 0x1 );

      // flag that we want all values within this key protected
      protectedItem.protectAllValues = TRUE;
      protectedItem.valueName[ 0 ]   = 0;
      swprintf( protectedItem.keyPath, L"System\\ControlSet%03d\\Services\\AppIntegrityDriver", controlSetIndex );
      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROTECTED_REG_ITEM, &protectedItem, sizeof( protectedItem ), NULL, 0, &bytesReturned, NULL );

      // protect the sub keys, Security and Enum
      swprintf( protectedItem.keyPath, L"System\\ControlSet%03d\\Services\\AppIntegrityDriver\\Security", controlSetIndex );
      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROTECTED_REG_ITEM, &protectedItem, sizeof( protectedItem ), NULL, 0, &bytesReturned, NULL );

      swprintf( protectedItem.keyPath, L"System\\ControlSet%03d\\Services\\AppIntegrityDriver\\Enum", controlSetIndex );
      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROTECTED_REG_ITEM, &protectedItem, sizeof( protectedItem ), NULL, 0, &bytesReturned, NULL );

      // protect the legacy folder
      swprintf( protectedItem.keyPath, L"System\\ControlSet%03d\\Enum\\Root\\LEGACY_APPINTEGRITYDRIVER", controlSetIndex );
      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROTECTED_REG_ITEM, &protectedItem, sizeof( protectedItem ), NULL, 0, &bytesReturned, NULL );

      swprintf( protectedItem.keyPath, L"System\\ControlSet%03d\\Enum\\Root\\LEGACY_APPINTEGRITYDRIVER\\0000", controlSetIndex );
      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROTECTED_REG_ITEM, &protectedItem, sizeof( protectedItem ), NULL, 0, &bytesReturned, NULL );

      swprintf( protectedItem.keyPath, L"System\\ControlSet%03d\\Enum\\Root\\LEGACY_APPINTEGRITYDRIVER\\0000\\Control", controlSetIndex );
      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROTECTED_REG_ITEM, &protectedItem, sizeof( protectedItem ), NULL, 0, &bytesReturned, NULL );
      

      // ---- Set which folders to protect ----
      char appPath[ MAX_PATH ];
      GetProgramPath( appPath );

      // skip the drive letter
      char *pFirstSlash = strchr( appPath, '\\' );
      pFirstSlash++;

      PROTECTED_DIR_INFO protectedDir = { 0 };
      MultiByteToWideChar( CP_ACP, 0, pFirstSlash, -1, protectedDir.dirPath, MAX_PATH );

      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROTECTED_DIR, &protectedDir, sizeof( protectedDir ), NULL, 0, &bytesReturned, NULL );


      // ---- Set which files to protect ----
      char systemDriversDir[ MAX_PATH ];
      if ( IsWindows64Bit( ) )
      {
         if ( !GetSystemWow64Directory( systemDriversDir, MAX_PATH ) )
         {
            // yeesh, our official call failed. Well, let's just hardcode it then.
            char windowsDir[ MAX_PATH ];
            GetWindowsDirectory( windowsDir, MAX_PATH );

            sprintf( systemDriversDir, "%s\\SysWOW64", windowsDir );
         }
      }
      else
      {
         SHGetFolderPath( NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, systemDriversDir );
      }

      char filePath[ MAX_PATH ];
      sprintf( filePath, "%s\\drivers\\appintegritydriver.sys", systemDriversDir );

      // skip the drive letter
      pFirstSlash = strchr( filePath, '\\' );
      pFirstSlash++;
      
      PROTECTED_FILE_INFO protectedFile = { 0 };
      MultiByteToWideChar( CP_ACP, 0, pFirstSlash, -1, protectedFile.filePath, MAX_PATH );

      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_SET_PROTECTED_FILE, &protectedFile, sizeof( protectedFile ), NULL, 0, &bytesReturned, NULL );


      // ---- activate the monitoring ----
      ULONG monitoringState = MONITOR_ALL_NT_SYSTEM_HOOKS;
      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_ACTIVATE_MONITORING, &monitoringState, sizeof( monitoringState ), NULL, 0, &bytesReturned, NULL );

      success = TRUE;
   }
   while( 0 );

   if ( success )
   {
      OutputDebugString( "Success: Enable Integrity\n" );
   }
   else
   {
      OutputDebugString( "Failed: Enabled Integrity\n" );
   }

   return success;
}

BOOL AppIntegrityUserMode::DisableIntegrity( void )
{
   if ( INVALID_HANDLE_VALUE != m_hDriver )
   {
      DWORD bytesReturned;
      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_CLEAR_PROTECTED_REG_LIST , NULL, 0, NULL, 0, &bytesReturned, NULL );
      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_CLEAR_PROTECTED_DIR_LIST , NULL, 0, NULL, 0, &bytesReturned, NULL );
      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_CLEAR_PROTECTED_FILE_LIST, NULL, 0, NULL, 0, &bytesReturned, NULL );

      // all this will do is turn off system hook monitoring so that the driver just passes everything thru.
      ULONG monitoring = MONITOR_ALL_NT_SYSTEM_HOOKS;
      DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_DEACTIVATE_MONITORING, &monitoring, sizeof( monitoring ), NULL, 0, &bytesReturned, NULL );

      // close our driver handle
      if ( !CloseHandle( m_hDriver ) )
      {
         OutputDebugString( "\tFailed to close Driver Handle\n" );
      }

      m_hDriver = INVALID_HANDLE_VALUE;
   }

   return TRUE;
}

BOOL AppIntegrityUserMode::EnableOptionsRegKey( void )
{
   // assume true incase the driver isn't installed
   BOOL result = TRUE;

   if ( INVALID_HANDLE_VALUE != m_hDriver )
   {
      // turn on setValueKey monitoring so all registry values are once again protected
      ULONG monitoring = MONITOR_NT_SET_VALUE_KEY;

      DWORD bytesReturned;
      result = DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_ACTIVATE_MONITORING, &monitoring, sizeof( monitoring ), NULL, 0, &bytesReturned, NULL );
   }

   return result;
}

BOOL AppIntegrityUserMode::DisableOptionsRegKey( void )
{
   // assume true incase the driver isn't installed
   BOOL result = TRUE;

   if ( INVALID_HANDLE_VALUE != m_hDriver )
   {
      // turn off setValueKey monitoring so options can be updated
      ULONG monitoring = MONITOR_NT_SET_VALUE_KEY;

      DWORD bytesReturned;
      result = DeviceIoControl( m_hDriver, IOCTL_APPINTEGRITY_DEACTIVATE_MONITORING, &monitoring, sizeof( monitoring ), NULL, 0, &bytesReturned, NULL );
   }

   return result;
}
