
#include "precompiled.h"
#include "finishPage.h"

FinishPage::FinishPage( void )
{
}

FinishPage::~FinishPage( )
{
   Destroy( );
}

BOOL FinishPage::Create( HWND parentHwnd )
{
   BOOL success = FALSE;

   do
   {
      m_Hwnd = CreateDialogParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_DIALOG_FINISHPAGE ), parentHwnd, (DLGPROC)FinishPage::Callback, (LPARAM)this );

      if ( !m_Hwnd ) break;

      success = TRUE;
   }
   while( 0 );

   return success;
}

void FinishPage::Destroy( void )
{
   if ( m_Hwnd )
   {
      DeleteObject( m_hBitmap );
      m_hBitmap = NULL;

      DestroyWindow( m_Hwnd );
      m_Hwnd = NULL;
   }
}
   
LRESULT CALLBACK FinishPage::Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   FinishPage *pFinishPage = (FinishPage *)GetWindowLong( hwnd, GWL_USERDATA );

   if ( pFinishPage || WM_INITDIALOG == uMsg )
   {
      switch( uMsg )
      {
         HANDLE_DLGMSG( hwnd, WM_INITDIALOG, FinishPage::FinishPage_WM_INITDIALOG );
         HANDLE_DLGMSG( hwnd, WM_COMMAND   , pFinishPage->FinishPage_WM_COMMAND   );
         HANDLE_DLGMSG( hwnd, WM_CLOSE     , pFinishPage->FinishPage_WM_CLOSE     );

         case WM_PAINT: return pFinishPage->FinishPage_WM_PAINT( hwnd );
      }
   }

   return FALSE;
}

BOOL FinishPage::FinishPage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam )
{
   FinishPage *pFinishPage = (FinishPage *)lParam;

   SetWindowLong( hwnd, GWL_USERDATA, (LONG)pFinishPage );

   return pFinishPage->FinishPage_WM_INITDIALOG( hwnd );
}

BOOL FinishPage::FinishPage_WM_INITDIALOG( HWND hwnd )
{
   m_Hwnd = hwnd;

   m_hBitmap = LoadBitmap( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDB_BITMAP5 ) );

   ShowWindow( hwnd, SW_SHOWNORMAL );

   return TRUE;
}

BOOL FinishPage::FinishPage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify )
{
   switch( id )
   {
      case ID_BUTTON_NEXT:
      case ID_BUTTON_FINISH:
      {
         PostMessage( GetParent( hwnd ), WM_FINISH_PRESSED, 0, 0 );
         break;
      }
   }

   return TRUE;
}

BOOL FinishPage::FinishPage_WM_PAINT( HWND hwnd )
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

BOOL FinishPage::FinishPage_WM_CLOSE( HWND hwnd )
{
   Destroy( );

   return TRUE;
}
