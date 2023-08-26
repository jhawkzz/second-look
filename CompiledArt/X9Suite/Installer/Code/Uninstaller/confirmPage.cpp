
#include "precompiled.h"
#include "confirmPage.h"
#include "..\\Shared\\InstallSettings.h"

ConfirmPage::ConfirmPage( void )
{
}

ConfirmPage::~ConfirmPage( )
{
   Destroy( );
}

BOOL ConfirmPage::Create( HWND parentHwnd )
{
   BOOL success = FALSE;

   do
   {
      m_Hwnd = CreateDialogParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_DIALOG_CONFIRMPAGE ), parentHwnd, (DLGPROC)ConfirmPage::Callback, (LPARAM)this );

      if ( !m_Hwnd ) break;

      success = TRUE;
   }
   while( 0 );

   return success;
}

void ConfirmPage::Destroy( void )
{
   if ( m_Hwnd )
   {
      DeleteObject( m_hBitmap );
      m_hBitmap = NULL;

      DestroyWindow( m_Hwnd );
      m_Hwnd = NULL;
   }
}
   
LRESULT CALLBACK ConfirmPage::Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   ConfirmPage *pConfirmPage = (ConfirmPage *)GetWindowLong( hwnd, GWL_USERDATA );

   if ( pConfirmPage || WM_INITDIALOG == uMsg )
   {
      switch( uMsg )
      {
         HANDLE_DLGMSG( hwnd, WM_INITDIALOG, ConfirmPage::ConfirmPage_WM_INITDIALOG );
         HANDLE_DLGMSG( hwnd, WM_COMMAND   , pConfirmPage->ConfirmPage_WM_COMMAND   );
         HANDLE_DLGMSG( hwnd, WM_CLOSE     , pConfirmPage->ConfirmPage_WM_CLOSE     );

         case WM_PAINT: return pConfirmPage->ConfirmPage_WM_PAINT( hwnd );
      }
   }

   return FALSE;
}

BOOL ConfirmPage::ConfirmPage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam )
{
   ConfirmPage *pConfirmPage = (ConfirmPage *)lParam;

   SetWindowLong( hwnd, GWL_USERDATA, (LONG)pConfirmPage );

   return pConfirmPage->ConfirmPage_WM_INITDIALOG( hwnd );
}

BOOL ConfirmPage::ConfirmPage_WM_INITDIALOG( HWND hwnd )
{
   m_Hwnd = hwnd;

   m_hBitmap = LoadBitmap( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDB_BITMAP6 ) );

   return TRUE;
}

BOOL ConfirmPage::ConfirmPage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify )
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

BOOL ConfirmPage::ConfirmPage_WM_PAINT( HWND hwnd )
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

BOOL ConfirmPage::ConfirmPage_WM_CLOSE( HWND hwnd )
{
   Destroy( );

   return TRUE;
}
