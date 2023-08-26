
#include "precompiled.h"
#include "settingsPage.h"

SettingsPage::SettingsPage( void )
{
}

SettingsPage::~SettingsPage( )
{
   Destroy( );
}

BOOL SettingsPage::Create( HWND parentHwnd )
{
   BOOL success = FALSE;

   do
   {
      m_Hwnd = CreateDialogParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_DIALOG_SETTINGSPAGE ), parentHwnd, (DLGPROC)SettingsPage::Callback, (LPARAM)this );

      if ( !m_Hwnd ) break;

      success = TRUE;
   }
   while( 0 );

   return success;
}

void SettingsPage::Destroy( void )
{
   if ( m_Hwnd )
   {
      DeleteObject( m_hBitmap );
      m_hBitmap = NULL;

      DestroyWindow( m_Hwnd );
      m_Hwnd = NULL;
   }
}
   
LRESULT CALLBACK SettingsPage::Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   SettingsPage *pSettingsPage = (SettingsPage *)GetWindowLong( hwnd, GWL_USERDATA );

   if ( pSettingsPage || WM_INITDIALOG == uMsg )
   {
      switch( uMsg )
      {
         HANDLE_DLGMSG( hwnd, WM_INITDIALOG, SettingsPage::SettingsPage_WM_INITDIALOG );
         HANDLE_DLGMSG( hwnd, WM_COMMAND   , pSettingsPage->SettingsPage_WM_COMMAND   );
         HANDLE_DLGMSG( hwnd, WM_CLOSE     , pSettingsPage->SettingsPage_WM_CLOSE     );

         case WM_PAINT:  return pSettingsPage->SettingsPage_WM_PAINT( hwnd );
         case WM_NOTIFY: return pSettingsPage->SettingsPage_WM_NOTIFY( hwnd, (int)wParam, (LPNMHDR)lParam );
      }
   }

   return FALSE;
}

BOOL SettingsPage::SettingsPage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam )
{
   SettingsPage *pSettingsPage = (SettingsPage *)lParam;

   SetWindowLong( hwnd, GWL_USERDATA, (LONG)pSettingsPage );

   return pSettingsPage->SettingsPage_WM_INITDIALOG( hwnd );
}

BOOL SettingsPage::SettingsPage_WM_INITDIALOG( HWND hwnd )
{
   m_Hwnd = hwnd;

   m_hBitmap = LoadBitmap( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDB_BITMAP3 ) );

   //default desktop and startmenu entries
   CheckDlgButton( hwnd, ID_CHECK_CREATE_DESKTOP_ICONS    , g_InstallSettings.m_InstallDesktopIcons   ? BST_CHECKED : BST_UNCHECKED );
   CheckDlgButton( hwnd, ID_CHECK_CREATE_STARTMENU_ENTRIES, g_InstallSettings.m_InstallStartMenuItems ? BST_CHECKED : BST_UNCHECKED );

   //whatever we chose, let's put that in the static control now
   SetInstallPathDisplay( g_InstallSettings.m_InstallDir );

   /*SendMessage( hwnd, DM_SETDEFID, ID_BUTTON_NEXT, 0 );

   LONG nextButtonStyle       = GetWindowLong( GetDlgItem( hwnd, ID_BUTTON_NEXT        ), GWL_STYLE );
   LONG destFolderButtonStyle = GetWindowLong( GetDlgItem( hwnd, ID_BUTTON_DEST_FOLDER ), GWL_STYLE );
   
   SendMessage( GetDlgItem( hwnd, ID_BUTTON_NEXT ), BM_SETSTYLE, nextButtonStyle | BS_DEFPUSHBUTTON, TRUE );
   SendMessage( GetDlgItem( hwnd, ID_BUTTON_DEST_FOLDER ), BM_SETSTYLE, destFolderButtonStyle & ~BS_DEFPUSHBUTTON, TRUE );*/

   return TRUE;
}

BOOL SettingsPage::SettingsPage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify )
{
   switch( id )
   {
      case ID_BUTTON_BACK:
      {
         PostMessage( GetParent( hwnd ), WM_BACK_PRESSED, 0, 0 );
         break;
      }

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

      case ID_CHECK_CREATE_DESKTOP_ICONS:
      {
         if ( BST_CHECKED == IsDlgButtonChecked( hwnd, id ) )
         {
            g_InstallSettings.m_InstallDesktopIcons = TRUE;
         }
         else
         {
            g_InstallSettings.m_InstallDesktopIcons = FALSE;
         }

         break;
      }

      case ID_CHECK_CREATE_STARTMENU_ENTRIES:
      {
         if ( BST_CHECKED == IsDlgButtonChecked( hwnd, id ) )
         {
            g_InstallSettings.m_InstallStartMenuItems = TRUE;
         }
         else
         {
            g_InstallSettings.m_InstallStartMenuItems = FALSE;
         }

         break;
      }

      case ID_BUTTON_DEST_FOLDER:
      {
         char currentPath[ MAX_PATH ];
         GetDlgItemText( hwnd, ID_STATIC_DEST_FOLDER, currentPath, MAX_PATH );

         // if they chose something, put it in the static control
         char newPath[ MAX_PATH ];
         if ( BrowseFolder( currentPath, newPath ) )
         {
            strcpy( g_InstallSettings.m_InstallDir, newPath );
            SetInstallPathDisplay( newPath );
         }
         break;
      }
   }

   return TRUE;
}

BOOL SettingsPage::SettingsPage_WM_NOTIFY( HWND hwnd, int idCtrl, LPNMHDR pnmh )
{
   return TRUE;
}

BOOL SettingsPage::SettingsPage_WM_PAINT( HWND hwnd )
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

BOOL SettingsPage::SettingsPage_WM_CLOSE( HWND hwnd )
{
   Destroy( );

   return TRUE;
}

BOOL SettingsPage::BrowseFolder( const char *pStartFolder, char *pDestFolder )
{
   BOOL success = FALSE;

   // get the PIDL for this dir and set the root
   LPSHELLFOLDER pDesktopFolder;
   LPITEMIDLIST  pidlStartFolder;
   OLECHAR       startFolderStr[ MAX_PATH ];

   // convert start folder to unipath
   MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pStartFolder, -1, startFolderStr, sizeof( startFolderStr ) );

   // get the desktop IShellFolder
   SHGetDesktopFolder( &pDesktopFolder );

   // convert startFolder to an Item ID list stored in pidlPath
   pDesktopFolder->ParseDisplayName( m_Hwnd, NULL, startFolderStr, NULL, &pidlStartFolder, NULL );

   BROWSEINFO browseInfo;
   memset( &browseInfo, 0, sizeof( BROWSEINFO ) );

   browseInfo.hwndOwner = m_Hwnd;
   browseInfo.lpfn      = SettingsPage::BrowseCallbackProc;
   browseInfo.ulFlags   = BIF_NEWDIALOGSTYLE;
   browseInfo.lParam    = (LPARAM) pidlStartFolder; //this will be used for the folder to start in
   browseInfo.lpszTitle = "Select or create an installation folder.";

   LPITEMIDLIST pidl = SHBrowseForFolder( &browseInfo );

   if ( pidl )
   {
      // get the name of the folder
      pDestFolder[ 0 ] = 0;
      SHGetPathFromIDList( pidl, pDestFolder );

      // if path is blank, they clicked on something like MyComputer, so we won't set success to true.
      if ( pDestFolder[ 0 ] )
      {
         // if there's a trailing slash (they picked C:\) chop it off.
         uint32 length = strlen( pDestFolder ) - 1;
         if ( '\\' == pDestFolder[ length ] )
         {
            pDestFolder[ length ] = NULL;
         }

         success = TRUE;
      }
   
      // free memory used
      IMalloc *pMalloc = NULL;

      if ( SUCCEEDED( SHGetMalloc( &pMalloc ) ) )
      {
         pMalloc->Free( pidl );
         pMalloc->Free( pidlStartFolder );
         pMalloc->Release ( );
         pDesktopFolder->Release( );
      }
   }

   return success;
}

int CALLBACK SettingsPage::BrowseCallbackProc( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData )
{
   switch( uMsg )
   {
      case BFFM_INITIALIZED:
      {
         //Set caption for Browse Folder
         SetWindowText( hwnd, "Select Installation Folder" );

         SendMessage( hwnd, BFFM_SETSELECTION, FALSE, lpData );

         break;
      }
   }
   return FALSE;
}

void SettingsPage::SetInstallPathDisplay( char *pPath )
{
   // make sure there's a trailing \ so it looks correct to the user.
   char installPath[ MAX_PATH ];
   sprintf( installPath, "%s\\", pPath );

   SetDlgItemText( m_Hwnd, ID_STATIC_DEST_FOLDER, installPath );
}
