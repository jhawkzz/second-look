
#include "precompiled.h"
#include "uninstallPage.h"
 
UninstallPage::UninstallPage( void )
{
}

UninstallPage::~UninstallPage( )
{
   Destroy( );
}

BOOL UninstallPage::Create( HWND parentHwnd )
{
   BOOL success = FALSE;

   do
   {
      m_Hwnd = CreateDialogParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_DIALOG_UNINSTALLPAGE ), parentHwnd, (DLGPROC)UninstallPage::Callback, (LPARAM)this );

      if ( !m_Hwnd ) break;

      // force a refresh before we install
      RedrawWindow( m_Hwnd, NULL, NULL, RDW_UPDATENOW );

      // uninstall, and if we fail let our parent know.
      if ( !UninstallProgram( ) )
      {
         SendMessage( GetParent( m_Hwnd ), WM_UNINSTALL_FAILED, 0, 0 );
      }

      EnableWindow( GetDlgItem( m_Hwnd, ID_BUTTON_NEXT ), TRUE );

      // set the focus and default status for the next button
      SetFocus( GetDlgItem( m_Hwnd, ID_BUTTON_NEXT ) );
      SendMessage( m_Hwnd, DM_SETDEFID, ID_BUTTON_NEXT, 0 );

      success = TRUE;
   }
   while( 0 );

   return success;
}

void UninstallPage::Destroy( void )
{
   if ( m_Hwnd )
   {
      DeleteObject( m_hBitmap );
      m_hBitmap = NULL;

      DestroyWindow( m_Hwnd );
      m_Hwnd = NULL;
   }
}
   
LRESULT CALLBACK UninstallPage::Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   UninstallPage *pUninstallPage = (UninstallPage *)GetWindowLong( hwnd, GWL_USERDATA );

   if ( pUninstallPage || WM_INITDIALOG == uMsg )
   {
      switch( uMsg )
      {
         HANDLE_DLGMSG( hwnd, WM_INITDIALOG, UninstallPage::UninstallPage_WM_INITDIALOG );
         HANDLE_DLGMSG( hwnd, WM_COMMAND   , pUninstallPage->UninstallPage_WM_COMMAND   );
         HANDLE_DLGMSG( hwnd, WM_CLOSE     , pUninstallPage->UninstallPage_WM_CLOSE     );

         case WM_PAINT:  return pUninstallPage->UninstallPage_WM_PAINT( hwnd );
         case WM_NOTIFY: return pUninstallPage->UninstallPage_WM_NOTIFY( hwnd, (int)wParam, (LPNMHDR)lParam );
      }
   }

   return FALSE;
}

BOOL UninstallPage::UninstallPage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam )
{
   UninstallPage *pUninstallPage = (UninstallPage *)lParam;

   SetWindowLong( hwnd, GWL_USERDATA, (LONG)pUninstallPage );

   return pUninstallPage->UninstallPage_WM_INITDIALOG( hwnd );
}

BOOL UninstallPage::UninstallPage_WM_INITDIALOG( HWND hwnd )
{
   m_Hwnd = hwnd;

   m_hBitmap = LoadBitmap( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDB_BITMAP8 ) );

   //Setup the progress bar
   SendMessage( GetDlgItem( hwnd, ID_PROGRESS_INSTALL ), PBM_SETRANGE, 0, MAKELPARAM( 0, 100 ) );

   //Disable the next button until the install is done.
   EnableWindow( GetDlgItem( hwnd, ID_BUTTON_NEXT ), FALSE );

   return TRUE;
}

BOOL UninstallPage::UninstallPage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify )
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

BOOL UninstallPage::UninstallPage_WM_NOTIFY( HWND hwnd, int idCtrl, LPNMHDR pnmh )
{
   return TRUE;
}

BOOL UninstallPage::UninstallPage_WM_PAINT( HWND hwnd )
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

BOOL UninstallPage::UninstallPage_WM_CLOSE( HWND hwnd )
{
   Destroy( );

   return TRUE;
}

BOOL UninstallPage::UninstallProgram( void )
{
   BOOL success = TRUE;

   char uninstallRegPath[ MAX_PATH ];
   sprintf( uninstallRegPath, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", IV_PROGRAM_NAME );

   // if we can't open the key, something horrible is happening.
   Registry_Class registry( HKEY_LOCAL_MACHINE );
   if ( !registry.Open_Key( uninstallRegPath, KEY_ALL_ACCESS, FALSE ) )
   {
      return FALSE;
   }

   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Removing Application Settings..." );
   g_InstallSettings.AppSpecificUninstall( );
   SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 12, 0 );
   
   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Removing Shortcuts..." );
   RemoveProgramShortcuts( );
   SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 25, 0 );

   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Removing Installed Packages..." );
   RemoveInstalledPackages( );
   SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 37, 0 );
   RedrawWindow( m_Hwnd, NULL, NULL, RDW_UPDATENOW );

   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Removing Registered Components..." );
   RemoveRegisteredDLLs( );
   SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 50, 0 );

   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Removing Files..." );
   RemoveFiles( &registry );
   SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 62, 0 );
   RedrawWindow( m_Hwnd, NULL, NULL, RDW_UPDATENOW );

   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Removing Folders..." );
   RemoveDirectories( &registry );   
   SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 75, 0 );

   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Removing Registry Keys..." );
   RemoveRegistryKeys( &registry );
   SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 87, 0 );

   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Removing File Assocations..." );
   RemoveFileAssociations( &registry );
   SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 95, 0 );
   

   // remove, of course, the uninstall registry key
   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Finalizing..." );

   registry.Set_Root_Key( HKEY_LOCAL_MACHINE );
   registry.Delete_Key( uninstallRegPath, FALSE );

   SendMessage( GetDlgItem( m_Hwnd, ID_PROGRESS_INSTALL ), PBM_SETPOS, 100, 0 );
   RedrawWindow( m_Hwnd, NULL, NULL, RDW_UPDATENOW );

   // all done
   SetDlgItemText( m_Hwnd, ID_STATIC_INSTALL_STATUS, "Uninstallation Complete" );

   return success;
}

void UninstallPage::RemoveProgramShortcuts( void )
{
   uint32 i;
   for ( i = 0; i < MAX_MAP_ENTRIES; i++ )
   {
      if ( g_InstallSettings.m_pShortcutMap[ i ].shortcutName[ 0 ] )
      {
         // if there's a subfolder for the shortcut, put that.
         char shortcutPath[ MAX_PATH ];
         if ( g_InstallSettings.m_pShortcutMap[ i ].shortcutSubDir[ 0 ] )
         {
            sprintf( shortcutPath, "%s\\%s", g_InstallSettings.m_pShortcutMap[ i ].shortcutSubDir, g_InstallSettings.m_pShortcutMap[ i ].shortcutName );
         }
         else
         {
            sprintf( shortcutPath, "%s", g_InstallSettings.m_pShortcutMap[ i ].shortcutName );
         }

         RemoveShortcut( g_InstallSettings.m_pShortcutMap[ i ].shortcutType, shortcutPath );
      }
   }
}

void UninstallPage::RemoveInstalledPackages( void )
{
   uint32 i;
   for ( i = 0; i < MAX_MAP_ENTRIES; i++ )
   {
      if ( g_InstallSettings.m_pInstallPackageIDMap[ i ][ 0 ] )
      {
         char runArguments[ MAX_PATH ];
         
         // remove the installed package
         sprintf( runArguments, "/x %s /q", g_InstallSettings.m_pInstallPackageIDMap[ i ] );
         
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
      }
   }
}

void UninstallPage::RemoveRegisteredDLLs( void )
{
   uint32 i;
   for ( i = 0; i < MAX_MAP_ENTRIES; i++ )
   {
      if ( g_InstallSettings.m_pDLLRegisterMap[ i ][ 0 ] )
      {
         // get the windows system32 folder
         char systemDir[ MAX_PATH ];
         SHGetFolderPath( NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, systemDir );
         
         // now unregister it with windows
         char dllFullFile[ MAX_PATH ];
         sprintf( dllFullFile, "%s\\%s", systemDir, g_InstallSettings.m_pDLLRegisterMap[ i ] );

         char registerCommand[ MAX_PATH ];
         sprintf( registerCommand, "%s /u /s", dllFullFile );
         
         SHELLEXECUTEINFO shellExecInfo = { 0 };
         shellExecInfo.fMask        = SEE_MASK_NOCLOSEPROCESS;
         shellExecInfo.cbSize       = sizeof( shellExecInfo );
         shellExecInfo.lpVerb       = "open";
         shellExecInfo.lpFile       = "regsvr32";
         shellExecInfo.nShow        = SW_SHOWNORMAL;
         shellExecInfo.lpParameters = registerCommand;
      
         ShellExecuteEx( &shellExecInfo );

         // wait for it to finish uninstalling before continuing
         WaitForSingleObject( shellExecInfo.hProcess, INFINITE );

         // now delete the file!
         DeleteFile( dllFullFile );
      }
   }
}

void UninstallPage::RemoveFiles( Registry_Class *pRegistry )
{
   // grab the installed folder path
   char uninstallPath[ MAX_PATH ];
   pRegistry->Get_Value_Data_String( "UninstallString", uninstallPath, MAX_PATH, "" );

   char *pEndSlash = strrchr( uninstallPath, '\\' );
   *pEndSlash = NULL; //chop da rest off.

   uint32 i;
   for ( i = 0; i < MAX_MAP_ENTRIES; i++ )
   {
      if ( g_InstallSettings.m_pInstallFileMap[ i ][ 0 ] )
      {  
         char fullFilePath[ MAX_PATH ];
         sprintf( fullFilePath, "%s\\%s", uninstallPath, g_InstallSettings.m_pInstallFileMap[ i ] );

         // now delete the file!
         DeleteFile( fullFilePath );
      }
   }

   // remove any extra files as well
   i = 0;
   while( g_InstallSettings.m_ExtraFilesToRemove[ i ][ 0 ] )
   {
      DeleteFile( g_InstallSettings.m_ExtraFilesToRemove[ i ] );

      i++;
   }
}

void UninstallPage::RemoveDirectories( Registry_Class *pRegistry )
{
   char registryEntry[ MAX_PATH ];
   char regValueStr[ MAX_PATH ];

   // let's count up all the folders to remove, we need to do it backwards.
   sint32 folderCount = 0;
   strcpy( registryEntry, "Folder0" );
   while( pRegistry->Get_Value_Data_String( registryEntry, regValueStr, MAX_PATH, "" ) )
   {
      folderCount++;
      sprintf( registryEntry, "Folder%d", folderCount );
   }

   // now remove each folder, but backwards since deleting a folder with a folder in it will fail.
   sint32 registryIndex = folderCount - 1;
   sprintf( registryEntry, "Folder%d", registryIndex );

   while( pRegistry->Get_Value_Data_String( registryEntry, regValueStr, MAX_PATH, "" ) )
   {
      // make sure the folder is empty. We'll have removed our files, but we want to leave user's files.
      if ( IsFolderEmpty( regValueStr ) )
      {
         RemoveDirectory( regValueStr );
      }

      registryIndex--;

      sprintf( registryEntry, "Folder%d", registryIndex );
   }
}

BOOL UninstallPage::IsFolderEmpty( const char *pSearchFolder )
{
   //Declare all needed handles
   WIN32_FIND_DATA FindFileData;
   HANDLE			 hFind;

   //Find the first file in the directory the user chose
   char fileName[ MAX_PATH ];
   sprintf( fileName, "%s\\*", pSearchFolder );

   hFind = FindFirstFile( fileName, &FindFileData );

   //Use a do/while so we process whatever FindFirstFile returned
   do
   {
      //Is it valid?
      if ( hFind != INVALID_HANDLE_VALUE )
      {
         //Is it a . or .. directory? If it is, skip, or we'll go forever.
         if ( !( strcmp( FindFileData.cFileName, "." ) ) || ! ( strcmp( FindFileData.cFileName, ".." ) ) )
         {
            continue;
         }

         // format our file
         sprintf( fileName, "%s\\%s", pSearchFolder, FindFileData.cFileName );

         // if this is another folder, search it
         if ( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
         {
            // if it fails, we fail
            if ( !IsFolderEmpty( fileName ) )
            {
               return FALSE;
            }
         }
         else
         {
            return FALSE;
         }
      }
   }
   while( FindNextFile( hFind, &FindFileData ) && hFind != INVALID_HANDLE_VALUE );

   FindClose( hFind );

   return TRUE;
}

void UninstallPage::RemoveRegistryKeys( Registry_Class *pRegistry )
{
   // remove any registry keys
   uint32 registryIndex = 0;
   while( g_InstallSettings.m_RegistryKeysToRemove[ registryIndex ][ 0 ] )
   {
      // what root key is it under?
      if ( strstr( g_InstallSettings.m_RegistryKeysToRemove[ registryIndex ], "HKEY_CLASSES_ROOT" ) )
      {  
         pRegistry->Set_Root_Key( HKEY_CLASSES_ROOT );
      }
      else if ( strstr( g_InstallSettings.m_RegistryKeysToRemove[ registryIndex ], "HKEY_LOCAL_MACHINE" ) )
      {
         pRegistry->Set_Root_Key( HKEY_LOCAL_MACHINE );
      }

      // skip past the root key
      char *pValueStr = strchr( g_InstallSettings.m_RegistryKeysToRemove[ registryIndex ], '\\' );
      pValueStr++;

      // now remove the key
      pRegistry->Delete_Key( pValueStr, TRUE );

      registryIndex++;
   }

   // remove any registry values
   registryIndex = 0;
   while( g_InstallSettings.m_RegistryValuesToRemove[ registryIndex ][ 0 ] )
   {
      // what root key is it under?
      if ( strstr( g_InstallSettings.m_RegistryValuesToRemove[ registryIndex ], "HKEY_CLASSES_ROOT" ) )
      {  
         pRegistry->Set_Root_Key( HKEY_CLASSES_ROOT );
      }
      else if ( strstr( g_InstallSettings.m_RegistryValuesToRemove[ registryIndex ], "HKEY_LOCAL_MACHINE" ) )
      {
         pRegistry->Set_Root_Key( HKEY_LOCAL_MACHINE );
      }

      // take just the key that the value is in
      char *pKeyStartStr = strchr ( g_InstallSettings.m_RegistryValuesToRemove[ registryIndex ], '\\' );
      char *pKeyEndStr   = strrchr( g_InstallSettings.m_RegistryValuesToRemove[ registryIndex ], '\\' );

      pKeyStartStr++;

      char registryKey[ MAX_PATH ] = { 0 };
      strncpy( registryKey, pKeyStartStr, pKeyEndStr - pKeyStartStr );

      // open the key that the value resides in
      pRegistry->Open_Key( registryKey, FALSE );

      // remove the value
      pKeyEndStr++; //skip past the slash
      pRegistry->Delete_Value( pKeyEndStr );

      registryIndex++;
   }

   // remove any registry values added by the installer
   registryIndex = 0;
   while( g_InstallSettings.m_RegistryValuesByInstaller[ registryIndex ].key[ 0 ])
   {  
      // what root key is it under?
      pRegistry->Set_Root_Key( g_InstallSettings.m_RegistryValuesByInstaller[ registryIndex ].rootKey );
      pRegistry->Open_Key( g_InstallSettings.m_RegistryValuesByInstaller[ registryIndex ].key, KEY_ALL_ACCESS );
      
      pRegistry->Delete_Value( g_InstallSettings.m_RegistryValuesByInstaller[ registryIndex ].value );
      
      registryIndex++;
   }
}

void UninstallPage::RemoveFileAssociations( Registry_Class *pRegistry )
{
   // and lastly restore any file types
   pRegistry->Set_Root_Key( HKEY_CLASSES_ROOT );

   uint32 registryIndex = 0;
   while( g_FileTypeList[ registryIndex ][ 0 ] )
   {
      if ( !strcmp( "AudioCD", g_FileTypeList[ registryIndex ] ) )
      {
         // audio cd is handled in a special way.
         pRegistry->Open_Key( "AudioCD\\shell\\play\\command", KEY_ALL_ACCESS );
   
         char valueData[ MAX_PATH ] = { 0 };
	      if ( pRegistry->Get_Value_Data_String( g_InstallSettings.m_FileTypeBackupValue, valueData, MAX_PATH, "" ) )
	      {
		      //We don't care about the result of this. If it failed that's not really our problem, our job is done.
            pRegistry->Set_Value_Data_String( NULL, valueData );
            pRegistry->Delete_Value( g_InstallSettings.m_FileTypeBackupValue );
	      }
      }
      else
      {
         pRegistry->Open_Key( g_FileTypeList[ registryIndex ], KEY_ALL_ACCESS );

         char valueStr[ MAX_PATH ];
         pRegistry->Get_Value_Data_String( NULL, valueStr, MAX_PATH, "" );

         // if we're still the owner, restore.

         // to see if we're the owner, go thru all the known values we use to claim ownership.
         // if we find a match, we own the type, so restore it and go to the next.
         uint32 fileTypeAssocIndex = 0;
         while( g_InstallSettings.m_FileTypeAssociationValue[ fileTypeAssocIndex ][ 0 ] )
         {
            if ( !strcmpi( valueStr, g_InstallSettings.m_FileTypeAssociationValue[ fileTypeAssocIndex ] ) )
            {
               valueStr[ 0 ] = 0;
               pRegistry->Get_Value_Data_String( g_InstallSettings.m_FileTypeBackupValue, valueStr, MAX_PATH, "" );
               pRegistry->Set_Value_Data_String( NULL, valueStr );
               pRegistry->Delete_Value( g_InstallSettings.m_FileTypeBackupValue );
               break;
            }

            fileTypeAssocIndex++;
         }
      }

      registryIndex++;
   }

   SHChangeNotify( SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0 );
}

BOOL UninstallPage::RemoveShortcut( int shortcutRootFolderCSIDL, const char *pShortcutFile )
{
   BOOL success = FALSE;

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

      char fullShortcut[ MAX_PATH ];
      sprintf( fullShortcut, "%s\\%s.lnk", shortcutRootFolder, pShortcutFile );

      DeleteFile( fullShortcut );
      
      DWORD error = GetLastError( );

      success = TRUE;
   }
   while( 0 );

   return success;
}
