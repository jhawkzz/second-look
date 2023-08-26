
#ifndef INSTALL_SETTINGS_H_
#define INSTALL_SETTINGS_H_

#include <stdio.h>
#include <shlobj.h>
#include <windows.h>

#include "types.h"

#pragma warning(disable : 4995) //Disable security warnings for stdio functions
#pragma warning(disable : 4996) //Disable security warnings for stdio functions
#define _CRT_SECURE_NO_DEPRECATE //Disable security warnings for stdio functions

// set your run once mutex here. (This should match what your program creates so the installer/uninstaller doesn't run while your program is in use.)
#define IV_RUN_ONCE_MUTEX_STRING "Second Look is already running."

// set the program name here.
#define IV_PROGRAM_NAME          "Second Look"

// set the uninstaller registry key here
#define IV_UNINSTALL_REG_KEY     "Second Look"

// set the default program folder name here
#define IV_DEFAULT_PROGRAM_FOLDER "Compiled Art\\Second Look"

// Add/Remove Program values 
#define IV_PUBLISHER_NAME        "Compiled Art"
#define IV_URL_INFO_ABOUT        "http://www.compiledart.com"
#define IV_URL_UPDATE_INFO       "http://www.compiledart.com"
#define IV_HELP_LINK             "http://www.compiledart.com"
#define IV_DISPLAY_VERSION       "1.20 Beta"

// macros for adding registry values
#define REG_INSTALL_APP_MACRO    "[InstallApp]"
#define REG_VALUE_TYPE_STRING    0x0
#define REG_VALUE_TYPE_KEY       0x1

#define MAX_MAP_ENTRIES (32768)
#define MAX_ENTRIES     (1024)

// windows version defines (for some reason the WinSDK doesn't define these)
#define WIN_9X_ME_NT_MAJOR                  (4)
#define WIN_2K_MAJOR                        (5)
#define WIN_2K_MINOR                        (0)
#define WIN_2K_XP_2K3_XP64_HOMESERVER_MAJOR (5)
#define WIN_VISTA_2K8_WIN7                  (6)

#define WIN_2K_MINOR                  (0)
#define WIN_XP_MINOR                  (1)
#define WIN_2K3_XP64_HOMESERVER_MINOR (2)

extern char g_FileTypeList[ ][ MAX_PATH ];

typedef struct _ShortcutObject
{
   char   shortcutTarget[ MAX_PATH ];

   char   shortcutName[ MAX_PATH ];
   char   shortcutSubDir[ MAX_PATH ];
   uint32 shortcutType; //like desktop, start menu, etc.
}
ShortcutObject;

typedef struct _RegistryValue
{
   UINT type;

   HKEY rootKey;
   char key[ MAX_PATH ];
   char value[ MAX_PATH ];
   char data[ MAX_PATH ];
}
RegistryValue;

class InstallSettings
{
   public:
                           InstallSettings ( void );
                           ~InstallSettings( );

          BOOL             Create      ( void );
          void             Destroy     ( void );

          void             GetDefaultInstallDir( void );
          BOOL             AppSpecificInstall( void );
          void             AppSpecificUninstall( void );

          BOOL             RestartSystem( void );

          BOOL             IsWindowsVersionSupported( void );
          BOOL             IsWindows64Bit( void );
          BOOL             IsWindowsXP( void );
          BOOL             IsWindowsVistaSP0( void );
          BOOL             IsWindowsVistaSP1Plus( void );
          BOOL             IsWindows7( void );

   private: 
          void             CreateInstallMaps( void );

   public:
          BOOL             m_EulaAgreed;
          BOOL             m_InstallDesktopIcons;
          BOOL             m_InstallStartMenuItems;
          BOOL             m_IsAppAlreadyInstalled;
          BOOL             m_NeedsRestart;
          char             m_InstallDir[ MAX_PATH ];
          char             m_UninstallRegPath[ MAX_PATH ];
          
          //These values are not user-configurable, they are set once at compile time.
          RegistryValue    m_RegistryValuesByInstaller[ MAX_ENTRIES ];
          char             m_RegistryKeysToRemove[ MAX_ENTRIES ][ MAX_PATH ];
          char             m_RegistryValuesToRemove[ MAX_ENTRIES ][ MAX_PATH ];
          char             m_FileTypeAssociationValue[ MAX_ENTRIES ][ MAX_PATH ];
          char             m_FileTypeBackupValue[ MAX_PATH ];
          char             m_ExtraFilesToRemove[ MAX_ENTRIES ][ MAX_PATH ];
          char             m_DisplayIconProgram[ MAX_PATH ];
          char             m_InstallFolder[ MAX_PATH ];
          char           **m_pInstallFileMap;
          char           **m_pDLLRegisterMap;
          char           **m_pInstallPackageMap;
          char           **m_pInstallPackageIDMap;
          
          ShortcutObject  *m_pShortcutMap;

   private:
          char            *m_pInstallFileMapData;
          char            *m_pDLLRegisterMapData;
          char            *m_pInstallPackageMapData;
          char            *m_pInstallPackageIDMapData;
};

#endif
