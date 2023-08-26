
#include "precompiled.h"
#include "about.h"
#include "options.h"

About::About( void )
{
   m_hwnd = NULL;
}

About::~About( )
{
}

BOOL About::Create( HWND parent_hwnd )
{
   // don't allow two instances
   if ( m_hwnd )
   {
      SetFocus( m_hwnd );

      return FALSE;
   }

   m_hwnd = CreateDialogParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_DIALOG_ABOUT ), parent_hwnd, (DLGPROC)Callback, (LPARAM) this );

   return TRUE;
}

LRESULT About::Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   About * p_about = (About *)ULongToPtr( GetWindowLong( hwnd, GWLP_USERDATA ) );

   if ( p_about || WM_INITDIALOG == uMsg )
   {
      switch( uMsg )
      {
         HANDLE_DLGMSG( hwnd, WM_INITDIALOG    , About::About_WM_INITDIALOG );
         HANDLE_DLGMSG( hwnd, WM_CTLCOLORSTATIC, p_about->About_WM_CTLCOLORSTATIC );
         HANDLE_DLGMSG( hwnd, WM_ERASEBKGND    , p_about->About_WM_ERASEBKGND );
         HANDLE_DLGMSG( hwnd, WM_MOUSEMOVE     , p_about->About_WM_MOUSEMOVE );
         HANDLE_DLGMSG( hwnd, WM_LBUTTONDOWN   , p_about->About_WM_LBUTTONDOWN );
         HANDLE_DLGMSG( hwnd, WM_LBUTTONUP     , p_about->About_WM_LBUTTONUP );
         HANDLE_DLGMSG( hwnd, WM_CLOSE         , p_about->About_WM_CLOSE    );
         HANDLE_DLGMSG( hwnd, WM_DESTROY       , p_about->About_WM_DESTROY  );
      }
   }

   return FALSE;
}

BOOL About::About_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam )
{
   About * p_about = (About *)lParam;

   SetWindowLong( hwnd, GWLP_USERDATA, PtrToLong(p_about) );

   return p_about->About_WM_INITDIALOG( hwnd );
}

BOOL About::About_WM_INITDIALOG( HWND hwnd )
{
   // create a font for the text
   LOGFONT log_font     = { 0 };
   log_font.lfQuality   = ANTIALIASED_QUALITY | PROOF_QUALITY;
   log_font.lfUnderline = FALSE;
   log_font.lfHeight    = 14;
   log_font.lfWeight    = FW_BOLD;

   strcpy( log_font.lfFaceName, "Arial" );

   m_hfont = CreateFontIndirect( &log_font );

   // assign the window font.
   SetWindowFont( GetDlgItem( hwnd, ID_STATIC_VERSION_STR ), m_hfont, TRUE );
   SetWindowFont( GetDlgItem( hwnd, ID_STATIC_VERSION     ), m_hfont, TRUE );
   SetWindowFont( GetDlgItem( hwnd, ID_STATIC_COPYRIGHT   ), m_hfont, TRUE );

   SetWindowText( GetDlgItem( hwnd, ID_STATIC_VERSION     ), X9_VERSION_STR );

   // load the logo
   m_hBitmap = LoadBitmap( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_BITMAP_ABOUT ) );

   ShowWindow( hwnd, SW_SHOWNORMAL );

   return TRUE;
}

BOOL About::About_WM_CTLCOLORSTATIC( HWND hwnd, HDC hdc, HWND hwndChild, int type )
{
   // is this the static website text?
   if ( GetDlgItem( hwnd, ID_STATIC_WEBSITE ) == hwndChild )
   {
      SetTextColor( hdc, RGB( 0, 0, 192 ) );

      SetBkMode( hdc, TRANSPARENT );

      return (BOOL) HandleToUlong(GetStockObject( HOLLOW_BRUSH ));
   }
   else
   {
      SetBkMode( hdc, TRANSPARENT );

      return (BOOL) HandleToUlong(GetStockObject( HOLLOW_BRUSH ));
   }

   return FALSE;
}

BOOL About::About_WM_ERASEBKGND( HWND hwnd, HDC hdc )
{
   RECT clientRect;
   GetClientRect( hwnd, &clientRect );

   // paint the bg white
   FillRect( hdc, &clientRect, (HBRUSH) GetStockObject( WHITE_BRUSH ) );

   HDC compatibleDC  = CreateCompatibleDC( hdc );
   HGDIOBJ oldBitmap = SelectObject( compatibleDC, m_hBitmap );

   BitBlt( hdc, 0, 0, 279, 174, compatibleDC, 0, 0, SRCCOPY );

   SelectObject( compatibleDC, oldBitmap );
   DeleteDC( compatibleDC );

   return TRUE;
}

BOOL About::About_WM_MOUSEMOVE( HWND hwnd, int x, int y, UINT keyFlags )
{
   return About_WM_LBUTTONDOWN( hwnd, FALSE, x, y, 0 );
}

BOOL About::About_WM_LBUTTONDOWN( HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags )
{
   RECT rect;
   GetWindowRect( GetDlgItem( hwnd, ID_STATIC_WEBSITE ), &rect );
   
   // can't use x/y because they are in client space.
   POINT cursor_pos;
   GetCursorPos( &cursor_pos );

   if ( PtInRect( &rect, cursor_pos ) )
   {
      SetCursor( LoadCursor( NULL, MAKEINTRESOURCE( IDC_HAND ) ) );
   }

   return TRUE;
}

BOOL About::About_WM_LBUTTONUP( HWND hwnd, int x, int y, UINT keyFlags )
{
   RECT rect;
   GetWindowRect( GetDlgItem( hwnd, ID_STATIC_WEBSITE ), &rect );

   // can't use x/y because they are in client space.
   POINT cursor_pos;
   GetCursorPos( &cursor_pos );

   // launch the website
   if ( PtInRect( &rect, cursor_pos ) )
   {
      ShellExecute( hwnd, "open", OptionsControl::CompanyDomain, NULL, NULL, SW_SHOWNORMAL );
   }

   return TRUE;
}

BOOL About::About_WM_CLOSE( HWND hwnd )
{
   if ( m_hBitmap )
   {
      DeleteObject( m_hBitmap );
   }

   if ( m_hfont )
   {
      DeleteObject( m_hfont );
   }

   if ( m_hwnd )
   {
      DestroyWindow( m_hwnd );

      m_hwnd = NULL;
   }

   return TRUE;
}

BOOL About::About_WM_DESTROY( HWND hwnd )
{
   SetForegroundWindow( GetParent( hwnd ) );

   return TRUE;
}
