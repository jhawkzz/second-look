
#include "precompiled.h"
#include "welcomePage.h"

WelcomePage::WelcomePage( void )
{
}

WelcomePage::~WelcomePage( )
{
   Destroy( );
}

BOOL WelcomePage::Create( HWND parentHwnd )
{
   BOOL success = FALSE;

   do
   {
      m_Hwnd = CreateDialogParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_DIALOG_WELCOMEPAGE ), parentHwnd, (DLGPROC)WelcomePage::Callback, (LPARAM)this );

      if ( !m_Hwnd ) break;

      success = TRUE;
   }
   while( 0 );

   return success;
}

void WelcomePage::Destroy( void )
{
   if ( m_Hwnd )
   {
      DeleteObject( m_hBitmap );
      m_hBitmap = NULL;

      DestroyWindow( m_Hwnd );
      m_Hwnd = NULL;
   }
}
   
LRESULT CALLBACK WelcomePage::Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   WelcomePage *pWelcomePage = (WelcomePage *)GetWindowLong( hwnd, GWL_USERDATA );

   if ( pWelcomePage || WM_INITDIALOG == uMsg )
   {
      switch( uMsg )
      {
         HANDLE_DLGMSG( hwnd, WM_INITDIALOG, WelcomePage::WelcomePage_WM_INITDIALOG );
         HANDLE_DLGMSG( hwnd, WM_COMMAND   , pWelcomePage->WelcomePage_WM_COMMAND   );
         HANDLE_DLGMSG( hwnd, WM_CLOSE     , pWelcomePage->WelcomePage_WM_CLOSE     );

         case WM_PAINT: return pWelcomePage->WelcomePage_WM_PAINT( hwnd );
      }
   }

   return FALSE;
}

BOOL WelcomePage::WelcomePage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam )
{
   WelcomePage *pWelcomePage = (WelcomePage *)lParam;

   SetWindowLong( hwnd, GWL_USERDATA, (LONG)pWelcomePage );

   return pWelcomePage->WelcomePage_WM_INITDIALOG( hwnd );
}

BOOL WelcomePage::WelcomePage_WM_INITDIALOG( HWND hwnd )
{
   m_Hwnd = hwnd;

   m_hBitmap = LoadBitmap( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDB_BITMAP1 ) );

   return TRUE;
}

BOOL WelcomePage::WelcomePage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify )
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

BOOL WelcomePage::WelcomePage_WM_PAINT( HWND hwnd )
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

BOOL WelcomePage::WelcomePage_WM_CLOSE( HWND hwnd )
{
   Destroy( );

   return TRUE;
}
