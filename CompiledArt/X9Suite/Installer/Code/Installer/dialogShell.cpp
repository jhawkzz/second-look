
#include "precompiled.h"
#include "dialogShell.h"
#include "welcomePage.h"
#include "eulaPage.h"
#include "settingsPage.h"
#include "installPage.h"
#include "finishPage.h"

DialogShell::DialogShell ( void )
{
   m_NoPromptToExit = FALSE;
   m_CurrentPage    = PageType_Welcome;
}

DialogShell::~DialogShell( )
{
}

BOOL DialogShell::Create( void )
{
   BOOL success = FALSE;

   do
   {
      m_Hwnd = CreateDialogParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_DIALOG_DIALOGSHELL ), NULL, (DLGPROC)DialogShell::Callback, (LPARAM) this );
      
      if ( !m_Hwnd ) break;

      success = TRUE;
   }
   while( 0 );

   return success;
}

void DialogShell::Destroy( void )
{
   // delete all old windows
   uint32 i;
   for ( i = 0; i < PageType_Count; i++ )
   {
      delete m_pPageList[ i ];
   }

   DestroyWindow( m_Hwnd );
   m_Hwnd = NULL;
}

IPage * DialogShell::GetCurrentPage( void )
{
   return m_pPageList[ m_CurrentPage ];
}

LRESULT CALLBACK DialogShell::Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   DialogShell *pDialogShell = (DialogShell *)GetWindowLong( hwnd, GWL_USERDATA );

   if ( pDialogShell || WM_INITDIALOG == uMsg )
   {
      switch( uMsg )
      {
         HANDLE_DLGMSG( hwnd, WM_INITDIALOG, DialogShell::DialogShell_WM_INITDIALOG );
         HANDLE_DLGMSG( hwnd, WM_COMMAND   , pDialogShell->DialogShell_WM_COMMAND );
         HANDLE_DLGMSG( hwnd, WM_CLOSE     , pDialogShell->DialogShell_WM_CLOSE   );
   
         case WM_BACK_PRESSED:   return pDialogShell->DialogShell_WM_BACK_PRESSED( hwnd );
         case WM_NEXT_PRESSED:   return pDialogShell->DialogShell_WM_NEXT_PRESSED( hwnd );
         case WM_CANCEL_PRESSED: return pDialogShell->DialogShell_WM_CANCEL_PRESSED( hwnd );
         case WM_FINISH_PRESSED: return pDialogShell->DialogShell_WM_FINISH_PRESSED( hwnd );
         case WM_INSTALL_FAILED: return pDialogShell->DialogShell_WM_INSTALL_FAILED( hwnd );
      }
   }

   return FALSE;
}

BOOL DialogShell::DialogShell_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam )
{
   DialogShell *pDialogShell = (DialogShell *)lParam;

   SetWindowLong( hwnd, GWL_USERDATA, (LONG)pDialogShell );

   return pDialogShell->DialogShell_WM_INITDIALOG( hwnd );
}

BOOL DialogShell::DialogShell_WM_INITDIALOG( HWND hwnd )
{
   m_Hwnd = hwnd;

   LoadSettings( );

   CreateWindows( );

   char captionStr[ MAX_PATH ];
   sprintf( captionStr, "%s Setup", IV_PROGRAM_NAME );
   SetWindowText( hwnd, captionStr );

   // launch the installer's first window
   m_pPageList[ m_CurrentPage ]->Create( hwnd );

   // disable the X button and close on the system menu. We will control exiting.
   HMENU hMenu = GetSystemMenu( hwnd, FALSE );

   EnableMenuItem( hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED | MF_DISABLED );

   DrawMenuBar( hwnd );
   // done

   HICON hIcon = LoadIcon( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ICON1 ) );

   SendMessage( hwnd, WM_SETICON, ICON_BIG  , (LPARAM) hIcon );
   SendMessage( hwnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon );
   
   ShowWindow( hwnd, SW_SHOWNORMAL );

   return TRUE;
}

BOOL DialogShell::DialogShell_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify )
{
   return TRUE;
}

BOOL DialogShell::DialogShell_WM_BACK_PRESSED( HWND hwnd )
{
   m_pPageList[ m_CurrentPage ]->Destroy( );

   m_CurrentPage = (PageType) ( (int) m_CurrentPage - 1 );

   m_pPageList[ m_CurrentPage ]->Create( hwnd );
   
   return TRUE;
}

BOOL DialogShell::DialogShell_WM_NEXT_PRESSED( HWND hwnd )
{
   m_pPageList[ m_CurrentPage ]->Destroy( );

   m_CurrentPage = (PageType) ( (int) m_CurrentPage + 1 );

   m_pPageList[ m_CurrentPage ]->Create( hwnd );
   
   return TRUE;
}

BOOL DialogShell::DialogShell_WM_CANCEL_PRESSED( HWND hwnd )
{
   m_NoPromptToExit = FALSE;

   PostMessage( hwnd, WM_CLOSE, 0, 0 );
   
   return TRUE;
}

BOOL DialogShell::DialogShell_WM_FINISH_PRESSED( HWND hwnd )
{
   m_NoPromptToExit = TRUE;

   PostMessage( hwnd, WM_CLOSE, 0, 0 );
   
   return TRUE;
}

BOOL DialogShell::DialogShell_WM_INSTALL_FAILED( HWND hwnd )
{
   m_NoPromptToExit = TRUE;

   PostMessage( hwnd, WM_CLOSE, 0, 0 );
   
   return TRUE;
}

BOOL DialogShell::DialogShell_WM_CLOSE( HWND hwnd )
{
   if ( m_NoPromptToExit || IDYES == MessageBox( hwnd, "Are you sure you wish to exit?", "Setup", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) )
   {
      SaveSettings( );

      // close the current page
      m_pPageList[ m_CurrentPage ]->Destroy( );

      PostQuitMessage( 0 );  
   }

   return TRUE;
}

void DialogShell::CreateWindows( void )
{
   //create our windows
   m_pPageList[ 0 ] = new WelcomePage;
   m_pPageList[ 1 ] = new EulaPage;
   m_pPageList[ 2 ] = new SettingsPage;
   m_pPageList[ 3 ] = new InstallPage;
   m_pPageList[ 4 ] = new FinishPage;
}

void DialogShell::LoadSettings( void )
{
}

void DialogShell::SaveSettings( void )
{
}
