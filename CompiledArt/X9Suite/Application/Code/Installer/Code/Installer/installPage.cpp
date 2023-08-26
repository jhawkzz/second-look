
#include "precompiled.h"
#include "installPage.h"
 
InstallPage::InstallPage( void )
{
   memset( m_InstallList    , 0, sizeof( m_InstallList ) );
   memset( m_ShortcutList   , 0, sizeof( m_ShortcutList ) );
   memset( m_RegistryList   , 0, sizeof( m_RegistryList ) );
   memset( m_DLLRegisterList, 0, sizeof( m_DLLRegisterList ) );
   memset( m_FolderList     , 0, sizeof( m_FolderList ) );

   m_RegistryCount    = 0;
   m_InstallCount     = 0;
   m_ShortcutCount    = 0;
   m_DLLRegisterCount = 0;
   m_FolderCount      = 0;
}

InstallPage::~InstallPage( )
{
   Destroy( );
}

BOOL InstallPage::Create( HWND parentHwnd )
{
   BOOL success = FALSE;

   do
   {
      m_Hwnd = CreateDialogParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_DIALOG_INSTALLPAGE ), parentHwnd, (DLGPROC)InstallPage::Callback, (LPARAM)this );

      if ( !m_Hwnd ) break;

      // force a refresh before we install
      RedrawWindow( m_Hwnd, NULL, NULL, RDW_UPDATENOW );

      // install, and if we fail let our parent know.
      if ( !InstallProgram( ) )
      {
         SendMessage( GetParent( m_Hwnd ), WM_INSTALL_FAILED, 0, 0 );
      }

      EnableWindow( GetDlgItem( m_Hwnd, ID_BUTTON_NEXT ), TRUE );

      success = TRUE;
   }
   while( 0 );

   return success;
}

void InstallPage::Destroy( void )
{
   if ( m_Hwnd )
   {
      DeleteObject( m_hBitmap );
      m_hBitmap = NULL;

      DestroyWindow( m_Hwnd );
      m_Hwnd = NULL;
   }
}
   
LRESULT CALLBACK InstallPage::Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   InstallPage *pInstallPage = (InstallPage *)GetWindowLong( hwnd, GWL_USERDATA );

   if ( pInstallPage || WM_INITDIALOG == uMsg )
   {
      switch( uMsg )
      {
         HANDLE_DLGMSG( hwnd, WM_INITDIALOG, InstallPage::InstallPage_WM_INITDIALOG );
         HANDLE_DLGMSG( hwnd, WM_COMMAND   , pInstallPage->InstallPage_WM_COMMAND   );
         HANDLE_DLGMSG( hwnd, WM_CLOSE     , pInstallPage->InstallPage_WM_CLOSE     );

         case WM_PAINT:  return pInstallPage->InstallPage_WM_PAINT( hwnd );
         case WM_NOTIFY: return pInstallPage->InstallPage_WM_NOTIFY( hwnd, (int)wParam, (LPNMHDR)lParam );
      }
   }

   return FALSE;
}

BOOL InstallPage::InstallPage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam )
{
   InstallPage *pInstallPage = (InstallPage *)lParam;

   SetWindowLong( hwnd, GWL_USERDATA, (LONG)pInstallPage );

   return pInstallPage->InstallPage_WM_INITDIALOG( hwnd );
}

BOOL InstallPage::InstallPage_WM_INITDIALOG( HWND hwnd )
{
   m_Hwnd = hwnd;

   m_hBitmap = LoadBitmap( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDB_BITMAP4 ) );

   //Setup the progress bar
   SendMessage( GetDlgItem( hwnd, ID_PROGRESS_INSTALL ), PBM_SETRANGE, 0, MAKELPARAM( 0, 100 ) );

   //Disable the next button until the install is done.
   EnableWindow( GetDlgItem( hwnd, ID_BUTTON_NEXT ), FALSE );

   return TRUE;
}

BOOL InstallPage::InstallPage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify )
{
   switch( id )
   {
      case ID_BUTTON_NEXT:
      {
         PostMessage( GetParent( hwnd ), WM_NEXT_PRESSED, 0, 0 );
         break;
      }

      case ID_BUTTON_CANCEL:
      {
         PostMessage( GetParent( hwnd ), WM_CANCEL_PRESSED, 0, 0 );
         break;
      }
   }

   return TRUE;
}

BOOL InstallPage::InstallPage_WM_NOTIFY( HWND hwnd, int idCtrl, LPNMHDR pnmh )
{
   return TRUE;
}

BOOL InstallPage::InstallPage_WM_PAINT( HWND hwnd )
{
   PAINTSTRUCT paintStruct;
   BeginPaint( hwnd, &paintStruct );

   HDC bmpDC         = CreateCompatibleDC( paintStruct.hdc );
   HGDIOBJ oldBitmap = SelectObject( bmpDC, m_hBitmap );

   BitBlt( paintStruct.hdc, 2, 1, PLUTO_WIDTH, PLUTO_HEIGHT, bmpDC, 0, 0, SRCCOPY );

   SelectObject( bmpDC, oldBitmap );
   DeleteDC( bmpDC );

   return TRUE;
}

BOOL InstallPage::InstallPage_WM_CLOSE( HWND hwnd )
{
   Destroy( );

   return TRUE;
}

BOOL InstallPage::InstallProgram( void )
{
   InstallError result = InstallError_Success;

   do
   {
      // open the registry
      Registry_Class registry( HKEY_LOCAL_MACHINE );
      registry.Open_Key( g_InstallSettings.m_UninstallRegPath, KEY_ALL_ACCESS );

      // update the status
      SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Installing Components..." );

      // install all files
      uint32 i;
      for ( i = 0; i < MAX_MAP_ENTRIES; i++ )
      {
         if ( g_InstallSettings.m_pInstallFileMap[ i ][ 0 ] )
         {
            if ( !InstallFile( i, m_InstallList[ m_InstallCount ], g_InstallSettings.m_InstallDir, g_InstallSettings.m_pInstallFileMap[ i ] ) )
            {
               result = InstallError_WriteFile;
               break;        
            }

            m_InstallCount++;
         }
      }
      if ( result != InstallError_Success ) break;
      SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 25, 0 );

      // install all packages. This should only be runtime style stuff that won't need to be rolled-back
      for ( i = 0; i < MAX_MAP_ENTRIES; i++ )
      {
         if ( g_InstallSettings.m_pInstallPackageMap[ i ][ 0 ] )
         {
            if ( !InstallPackage( i, g_InstallSettings.m_pInstallPackageMap[ i ] ) )
            {
               result = InstallError_WriteFile;
               break;
            }
         }
      }
      if ( result != InstallError_Success ) break;
      SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 35, 0 );

      // force a refresh of the window
      RedrawWindow( m_Hwnd, NULL, NULL, RDW_UPDATENOW );

      // install & register DLLs
      SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Registering Components..." );

      // get the windows system32 folder
      char systemDir[ MAX_PATH ];
      SHGetFolderPath( NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, systemDir );

      for ( i = 0; i < MAX_MAP_ENTRIES; i++ )
      {
         if ( g_InstallSettings.m_pDLLRegisterMap[ i ][ 0 ] )
         {
            if ( !InstallFile( i, m_DLLRegisterList[ m_DLLRegisterCount ], systemDir, g_InstallSettings.m_pDLLRegisterMap[ i ] ) )
            {
               result = InstallError_WriteFile;
               break;        
            }
            
            // now register it with windows
            char dllFullFile[ MAX_PATH ];
            sprintf( dllFullFile, "%s\\%s", systemDir, g_InstallSettings.m_pDLLRegisterMap[ i ] );

            char registerCommand[ MAX_PATH ];
            sprintf( registerCommand, "%s /s", dllFullFile );

            ShellExecute( m_Hwnd, "open", "regsvr32", registerCommand, NULL, SW_HIDE );

            m_DLLRegisterCount++;
         }
      }
      if ( result != InstallError_Success ) break;
      SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 50, 0 );

      // force a refresh of the window
      RedrawWindow( m_Hwnd, NULL, NULL, RDW_UPDATENOW );

      // add shortcuts
      SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Installing Shortcuts..." );

      i = 0;
      while( g_InstallSettings.m_pShortcutMap[ i ].shortcutName[ 0 ] )
      {
         // if it's a start menu item and we're allowed, or a desktop item and we're allowed, add it.
         if ( (CSIDL_PROGRAMS         == g_InstallSettings.m_pShortcutMap[ i ].shortcutType && g_InstallSettings.m_InstallStartMenuItems) ||
              (CSIDL_DESKTOPDIRECTORY == g_InstallSettings.m_pShortcutMap[ i ].shortcutType && g_InstallSettings.m_InstallDesktopIcons) )
         {
            if ( !CreateShortcut( g_InstallSettings.m_pShortcutMap[ i ].shortcutType, 
                                  m_ShortcutList[ m_ShortcutCount ],
                                  g_InstallSettings.m_InstallDir, 
                                  g_InstallSettings.m_pShortcutMap[ i ].shortcutTarget, 
                                  g_InstallSettings.m_pShortcutMap[ i ].shortcutSubDir, 
                                  g_InstallSettings.m_pShortcutMap[ i ].shortcutName ) )
            {
               result = InstallError_WriteShortcut;
               break;
            }

            m_ShortcutCount++;
         }

         i++;
      }
      if ( result != InstallError_Success ) break;
      SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 75, 0 );

      // force a refresh of the window
      RedrawWindow( m_Hwnd, NULL, NULL, RDW_UPDATENOW );

      // time to let Windows know this program is installed
      SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Updating Registry..." );

      char installFile[ MAX_PATH ];
      sprintf( installFile, "%s\\%s", g_InstallSettings.m_InstallDir, g_InstallSettings.m_DisplayIconProgram );

      char uninstallFile[ MAX_PATH ];
      sprintf( uninstallFile, "%s\\uninstall.exe", g_InstallSettings.m_InstallDir );

      registry.Set_Value_Data_String( "DisplayName"          , IV_PROGRAM_NAME );
      registry.Set_Value_Data_String( "Publisher"            , IV_PUBLISHER_NAME );
      registry.Set_Value_Data_String( "URLInfoAbout"         , IV_URL_INFO_ABOUT );
      registry.Set_Value_Data_String( "URLUpdateInfo"        , IV_URL_UPDATE_INFO );
      registry.Set_Value_Data_String( "HelpLink"             , IV_HELP_LINK );
      registry.Set_Value_Data_String( "DisplayVersion"       , IV_DISPLAY_VERSION );
      registry.Set_Value_Data_String( "DisplayIcon"          , installFile );
      registry.Set_Value_Data_String( "UninstallString"      , uninstallFile );
      registry.Set_Value_Data_Int   ( "NoModify"             , 1 );
      registry.Set_Value_Data_Int   ( "NoRepair"             , 1 );
      registry.Set_Value_Data_Int   ( "InstallDesktopIcons"  , g_InstallSettings.m_InstallDesktopIcons   );
      registry.Set_Value_Data_Int   ( "InstallStartMenuItems", g_InstallSettings.m_InstallStartMenuItems );

      strcpy( m_RegistryList[ m_RegistryCount++ ], "DisplayName" );
      strcpy( m_RegistryList[ m_RegistryCount++ ], "Publisher" );
      strcpy( m_RegistryList[ m_RegistryCount++ ], "URLInfoAbout" );
      strcpy( m_RegistryList[ m_RegistryCount++ ], "URLUpdateInfo" );
      strcpy( m_RegistryList[ m_RegistryCount++ ], "HelpLink" );
      strcpy( m_RegistryList[ m_RegistryCount++ ], "DisplayVersion" );
      strcpy( m_RegistryList[ m_RegistryCount++ ], "DisplayIcon" );
      strcpy( m_RegistryList[ m_RegistryCount++ ], "UninstallString" );
      strcpy( m_RegistryList[ m_RegistryCount++ ], "NoModify" );
      strcpy( m_RegistryList[ m_RegistryCount++ ], "NoRepair" );
      strcpy( m_RegistryList[ m_RegistryCount++ ], "InstallDesktopIcons" );
      strcpy( m_RegistryList[ m_RegistryCount++ ], "InstallStartMenuItems" );

      // put registry entries for all the folders we created
      for ( i = 0; i < m_FolderCount; i++ )
      {
         char registryEntry[ MAX_PATH ];
         sprintf( registryEntry, "Folder%d", i );

         registry.Set_Value_Data_String( registryEntry, m_FolderList[ i ] );
      }

      // allow the application to do custom installation stuff
      g_InstallSettings.AppSpecificInstall( );

      i = 0;
      while( g_InstallSettings.m_RegistryValuesByInstaller[ i ].key[ 0 ] )
      {
         AddRegistryValue( &g_InstallSettings.m_RegistryValuesByInstaller[ i ] );
         i++;
      }
   }
   while( 0 );


   // update the final status
   switch( result )
   {
      case InstallError_Success:
      {
         SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Installation Complete" );
         SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 100, 0 );
         
         // enable the next button, since we're now done.
         EnableWindow( GetDlgItem( m_Hwnd, ID_BUTTON_NEXT ), TRUE );
         
         // set the focus and default status for the next button
         SetFocus( GetDlgItem( m_Hwnd, ID_BUTTON_NEXT ) );
         SendMessage( m_Hwnd, DM_SETDEFID, ID_BUTTON_NEXT, 0 );
         return TRUE;
      }

      case InstallError_WriteShortcut:
      {
         HandleInstallError( "Failed to Install Files. Check Available Hard Drive Space." );
         return FALSE;
      }

      case InstallError_WriteFile:
      {
         HandleInstallError( "Failed to Install Files. Check Available Hard Drive Space." );
         return FALSE;
      }

      case InstallError_RegisterDLL:
      {
         HandleInstallError( "Failed to Install Files. Check Available Hard Drive Space." );
         return FALSE;
      }
   }

   return FALSE;
}

void InstallPage::HandleInstallError( const char *pInstallError )
{
   char errorMsg[ MAX_PATH ];

   if ( !g_InstallSettings.m_IsAppAlreadyInstalled )
   {
      sprintf( errorMsg, "Setup failed with the following error: %s\r\nThe installation will now attempt to roll back.", pInstallError );

      SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Installation Failed" );

      MessageBox( m_Hwnd, errorMsg, "Setup Error", MB_OK | MB_ICONERROR );

      if ( RollbackInstall( ) )
      {
         MessageBox( m_Hwnd, "Setup has rolled back the installation.\r\nSetup will now exit.", "Setup Error", MB_OK | MB_ICONINFORMATION );   
      }
      else
      {
         MessageBox( m_Hwnd, "Setup has not completely rolled back the installation.\r\nSome installation components may remain. Please try to install again.", "Setup Error", MB_OK | MB_ICONINFORMATION );
      }
   }
   else
   {
      sprintf( errorMsg, "Setup failed with the following error: %s\r\nSince this program has already been installed, no roll back will be attempted.\r\nSetup will now exit.", pInstallError );

      MessageBox( m_Hwnd, errorMsg, "Setup Error", MB_OK | MB_ICONERROR );

      SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Installation Failed" );
   }
}

BOOL InstallPage::RollbackInstall( void )
{
   BOOL success = TRUE;

   Registry_Class registry( HKEY_LOCAL_MACHINE );
   registry.Open_Key( g_InstallSettings.m_UninstallRegPath, KEY_ALL_ACCESS );

   uint32 i;

   // remove registry entries
   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Removing Registry Entries..." );

   for ( i = 0; i < m_RegistryCount; i++ )
   {
      success &= registry.Delete_Value( m_RegistryList[ i ] );
   }

   if ( !registry.Get_Key_Value_Count( ) )
   {
      success &= registry.Delete_Key( g_InstallSettings.m_UninstallRegPath, FALSE );
   }


   // remove shortcuts
   for ( i = 0; i < m_ShortcutCount; i++ )
   {
      success &= (BOOL) DeleteFile( m_ShortcutList[ i ] );
   }
   SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 75, 0 );

   // force a refresh of the window
   RedrawWindow( m_Hwnd, NULL, NULL, RDW_UPDATENOW );


   // unregister DLLs
   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Removing Registered Components..." );

   for ( i = 0; i < m_DLLRegisterCount; i++ )
   {
      char registerCommand[ MAX_PATH ];
      sprintf( registerCommand, "%s /s /u", m_DLLRegisterList[ i ] );

      ShellExecute( m_Hwnd, "open", "regsvr32", registerCommand, NULL, SW_HIDE );

      success &= (BOOL) DeleteFile( m_DLLRegisterList[ i ] );
   }
   SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 50, 0 );

   // force a refresh of the window
   RedrawWindow( m_Hwnd, NULL, NULL, RDW_UPDATENOW );


   // remove any existing files
   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Removing Installation Files..." );

   for ( i = 0; i < m_InstallCount; i++ )
   {
      success &= (BOOL) DeleteFile( m_InstallList[ i ] );
   }
   SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 25, 0 );
   
   // force a refresh of the window
   RedrawWindow( m_Hwnd, NULL, NULL, RDW_UPDATENOW );


   
   // remove any existing folders
   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Removing Created Directories..." );

   for ( i = 0; i < m_FolderCount; i++ )
   {
      //guaruntee double null terminated
      char folderDeletePath[ MAX_PATH * 2 ] = { 0 };
      strcpy( folderDeletePath, m_FolderList[ i ] );

      SHFILEOPSTRUCT shFileOp = { 0 };
      shFileOp.hwnd   = m_Hwnd;
      shFileOp.wFunc  = FO_DELETE;
      shFileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;
      shFileOp.pFrom  = folderDeletePath;

      // don't check this since we could attempt a delete on 
      // a sub folder, and worst case we'd just leave behind an empty folder
      SHFileOperation( &shFileOp );
   }
   SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 0, 0 );

   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Rollback Complete" );

   return success;
}

BOOL InstallPage::InstallPackage( UINT id, const char *pInstallName )
{
   BOOL success      = FALSE;
   HGLOBAL hResource = NULL;
   FILE *pFile       = NULL;

   do
   {
      // First acquire the resource
      HRSRC hrSrc = FindResource( GetModuleHandle( NULL ), MAKEINTRESOURCE( id ), "RT_RCDATA" );
      hResource   = LoadResource( GetModuleHandle( NULL ), hrSrc );

      if ( !hResource ) break;
      
      // get a pointer to the data
      BYTE *pResourceData = (BYTE *)LockResource( hResource );
      DWORD resourceSize  = SizeofResource( GetModuleHandle( NULL ), hrSrc );

      // get a temp folder from windows..I can't believe it. It's one. simple. call.
      char tempPath[ MAX_PATH ];
      GetTempPath( MAX_PATH, tempPath );

      // as I just learned thanks to NT 4, it's POSSIBLE that the temp folder doesn't exist, so try to make it
      CreateDirectory( tempPath, NULL );

      // build the full filename
      char fullFileInstallPath[ MAX_PATH ];
      sprintf( fullFileInstallPath, "%s%s", tempPath, pInstallName );

      // try to open it for write,

      // overwrite any readonly setting.
      if ( -1 == _access( fullFileInstallPath, 02 ) )
      {
         _chmod( fullFileInstallPath, _S_IWRITE );
      }

      pFile = fopen( fullFileInstallPath, "wb" );

      if ( !pFile ) break;

      // attempt to write the file.
      if ( fwrite( pResourceData, 1, resourceSize, pFile ) != resourceSize ) break;

      fclose( pFile );
      pFile = NULL;

      
      char runArguments[ MAX_PATH ];
      
      // first remove any existing version
      sprintf( runArguments, "/x %s /q", fullFileInstallPath );
      
      SHELLEXECUTEINFO shellExecInfo = { 0 };
      shellExecInfo.fMask        = SEE_MASK_NOCLOSEPROCESS;
      shellExecInfo.cbSize       = sizeof( shellExecInfo );
      shellExecInfo.lpVerb       = "open";
      shellExecInfo.lpFile       = "msiexec";
      shellExecInfo.nShow        = SW_SHOWNORMAL;
      shellExecInfo.lpParameters = runArguments;
      
      ShellExecuteEx( &shellExecInfo );

      // wait for it to finish uninstalling before continuing
      WaitForSingleObject( shellExecInfo.hProcess, INFINITE );

      // add new version
      sprintf( runArguments, "/i %s /q", fullFileInstallPath );
      ShellExecuteEx( &shellExecInfo );

      // wait for it to finish INstalling before continuing
      WaitForSingleObject( shellExecInfo.hProcess, INFINITE );
     
      success = TRUE;
   }
   while( 0 );

   // release the resource
   if ( pFile )
   {
      fclose( pFile );
   }

   if ( hResource )
   {
      FreeResource( hResource );
   }

   return success;
}

BOOL InstallPage::InstallFile( UINT id, char *pInstallLogFileStr, const char *pInstallDir, const char *pInstallName )
{
   BOOL success      = FALSE;
   HGLOBAL hResource = NULL;
   FILE *pFile       = NULL;

   do
   {
      // First acquire the resource
      HRSRC hrSrc = FindResource( GetModuleHandle( NULL ), MAKEINTRESOURCE( id ), "RT_RCDATA" );
      hResource   = LoadResource( GetModuleHandle( NULL ), hrSrc );

      if ( !hResource ) break;
      
      // get a pointer to the data
      BYTE *pResourceData = (BYTE *)LockResource( hResource );
      DWORD resourceSize  = SizeofResource( GetModuleHandle( NULL ), hrSrc );

      // build the full filename
      char fullFileInstallPath[ MAX_PATH ];
      sprintf( fullFileInstallPath, "%s\\%s", pInstallDir, pInstallName );

      // try to open it for write,
      
      // overwrite any read only setting.
      if ( -1 == _access( fullFileInstallPath, 02 ) )
      {
         _chmod( fullFileInstallPath, _S_IWRITE );
      }
      
      // try opening it
      pFile = fopen( fullFileInstallPath, "wb" );

      if ( !pFile )
      {
         // it's possible the path doesn't exist. Let's try making it.
         char filePath[ MAX_PATH ];
         strcpy( filePath, fullFileInstallPath );

         char *pLastPath = strrchr( filePath, '\\' );
         if ( !pLastPath ) break;

         // terminate at the filename
         *(pLastPath + 1) = NULL;

         // call create for each folder we encounter
         char *pNextFolder = strchr( filePath, '\\' );
         
         while( pNextFolder )
         {
            char currentFolder[ MAX_PATH ] = { 0 };
            strncpy( currentFolder, filePath, pNextFolder - filePath + 1 );

            if ( CreateDirectory( currentFolder, NULL ) )
            {
               // log the file folder we created
               strcpy( m_FolderList[ m_FolderCount++ ], currentFolder );
            }

            pNextFolder = strchr( pNextFolder + 1, '\\' );
         }

         // try opening it
         pFile = fopen( fullFileInstallPath, "wb" );
      }

      if ( !pFile ) break;

      // attempt to write the file.
      if ( fwrite( pResourceData, 1, resourceSize, pFile ) != resourceSize ) break;

      // log our file
      if ( pInstallLogFileStr )
      {
         strcpy( pInstallLogFileStr, fullFileInstallPath );
      }

      success = TRUE;
   }
   while( 0 );

   // release the resource
   if ( pFile )
   {
      fclose( pFile );
   }

   if ( hResource )
   {
      FreeResource( hResource );
   }

   return success;
}


BOOL InstallPage::CreateShortcut( int shortcutRootFolderCSIDL, char *pInstallLogFileStr, const char *pTargetDir, const char *pTargetName, const char *pShortcutSubDir, const char *pShortcutName )
{
   BOOL success               = FALSE;
   IShellLink *pShellLink     = NULL;
   IPersistFile* pPersistFile = NULL; 

   CoInitialize( NULL );

   do
   {
      // get the ID of the special folder
      LPITEMIDLIST lpItemIdList;
      if ( SHGetSpecialFolderLocation( NULL, shortcutRootFolderCSIDL, &lpItemIdList ) != S_OK ) break;

      // extract the actual path 
      char shortcutRootFolder[ MAX_PATH ];
      if ( !SHGetPathFromIDList( lpItemIdList, shortcutRootFolder ) ) break;

      // free the pidl
      LPMALLOC pMalloc = NULL;
      if ( SHGetMalloc( &pMalloc ) != S_OK ) break;

      pMalloc->Free( lpItemIdList );
      pMalloc->Release( );
   
      // get a pointer to the IShellLink interface. 
      if ( CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, ( void **)&pShellLink ) != S_OK ) break;
      
      // create the full target path and apply it to the shortcut
      char fullTargetPath[ MAX_PATH ];
      sprintf( fullTargetPath, "%s\\%s", pTargetDir, pTargetName );

      if ( pShellLink->SetPath( fullTargetPath ) != S_OK ) break;

      // get the persistent file interface
      if ( pShellLink->QueryInterface( IID_IPersistFile, ( void **)&pPersistFile ) != S_OK ) break;

      // build the shortcut name
      char shortcutFile[ MAX_PATH ];
      sprintf( shortcutFile, "%s\\%s\\%s.lnk", shortcutRootFolder, pShortcutSubDir, pShortcutName );

      // make sure the folder where the shortcut is going to go exists. if this is say, in the start menu, it might not
      char fullShortcutPath[ MAX_PATH ];
      sprintf( fullShortcutPath, "%s\\%s", shortcutRootFolder, pShortcutSubDir );
      if ( CreateDirectory( fullShortcutPath, NULL ) )
      {
         // log the install
         strcpy( m_FolderList[ m_FolderCount++ ], fullShortcutPath );
      }

      if ( pInstallLogFileStr )
      {
         strcpy( pInstallLogFileStr, shortcutFile );
      }

      // convert to wide
      WCHAR wShortcutFile[ MAX_PATH ]; 
      MultiByteToWideChar( CP_ACP, 0, shortcutFile, -1, wShortcutFile, MAX_PATH );

      if ( pPersistFile->Save( wShortcutFile, TRUE ) != S_OK ) break;

      success = TRUE;
   }
   while( 0 );
   
   if ( pPersistFile )
   {
      pPersistFile->Release( );
   }

   if ( pShellLink )
   {
      pShellLink->Release( );
   }

   CoUninitialize( );

   return success;
}

void InstallPage::AddRegistryValue( RegistryValue *pRegValue )
{
   Registry_Class registry( pRegValue->rootKey );

   switch( pRegValue->type )
   {
      case REG_VALUE_TYPE_STRING:
      {
         registry.Open_Key( pRegValue->key, KEY_ALL_ACCESS, TRUE );

         // build the value to use. check for macros first
         char registryData[ MAX_PATH ];
         if ( !strcmp( REG_INSTALL_APP_MACRO, pRegValue->data ) )
         {
            sprintf( registryData, "%s\\%s", g_InstallSettings.m_InstallDir, g_InstallSettings.m_DisplayIconProgram );
         }
         else
         {
            strcpy( registryData, pRegValue->data );
         }

         registry.Set_Value_Data_String( pRegValue->value, registryData );
         break;
      }

      case REG_VALUE_TYPE_KEY:
      {
         registry.Create_Key( pRegValue->key );
         break;
      }
   }
}
