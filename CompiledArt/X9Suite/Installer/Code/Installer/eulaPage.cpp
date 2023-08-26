
#include "precompiled.h"
#include "eulaPage.h"

EulaPage::EulaPage( void )
{
}

EulaPage::~EulaPage( )
{
   Destroy( );
}

BOOL EulaPage::Create( HWND parentHwnd )
{
   BOOL success = FALSE;

   do
   {
      m_Hwnd = CreateDialogParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_DIALOG_EULAPAGE ), parentHwnd, (DLGPROC)EulaPage::Callback, (LPARAM)this );

      if ( !m_Hwnd ) break;

      success = TRUE;
   }
   while( 0 );

   return success;
}

void EulaPage::Destroy( void )
{
   if ( m_Hwnd )
   {
      DeleteObject( m_hBitmap );
      m_hBitmap = NULL;

      DestroyWindow( m_Hwnd );
      m_Hwnd = NULL;
   }
}
   
LRESULT CALLBACK EulaPage::Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   EulaPage *pEulaPage = (EulaPage *)GetWindowLong( hwnd, GWL_USERDATA );

   if ( pEulaPage || WM_INITDIALOG == uMsg )
   {
      switch( uMsg )
      {
         HANDLE_DLGMSG( hwnd, WM_INITDIALOG, EulaPage::EulaPage_WM_INITDIALOG );
         HANDLE_DLGMSG( hwnd, WM_COMMAND   , pEulaPage->EulaPage_WM_COMMAND   );
         //HANDLE_DLGMSG( hwnd, WM_CLOSE     , pEulaPage->EulaPage_WM_CLOSE     ); <--Rich Edit controls are buggy, and if you handle this it will kill the rich edit if escape is pressed with it in focus.

         case WM_PAINT:  return pEulaPage->EulaPage_WM_PAINT( hwnd );
         case WM_NOTIFY: return pEulaPage->EulaPage_WM_NOTIFY( hwnd, (int)wParam, (LPNMHDR)lParam );
      }
   }

   return FALSE;
}

BOOL EulaPage::EulaPage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam )
{
   EulaPage *pEulaPage = (EulaPage *)lParam;

   SetWindowLong( hwnd, GWL_USERDATA, (LONG)pEulaPage );

   return pEulaPage->EulaPage_WM_INITDIALOG( hwnd );
}

BOOL EulaPage::EulaPage_WM_INITDIALOG( HWND hwnd )
{
   m_Hwnd = hwnd;

   m_hBitmap = LoadBitmap( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDB_BITMAP2 ) );

   // default the EULA agreement to no.
   if ( g_InstallSettings.m_EulaAgreed )
   {
      EnableWindow( GetDlgItem( hwnd, ID_BUTTON_NEXT ), TRUE );
      CheckDlgButton( hwnd, ID_RADIO_EULA_AGREE, BST_CHECKED );
   }
   else
   {
      EnableWindow( GetDlgItem( hwnd, ID_BUTTON_NEXT ), FALSE );
      CheckDlgButton( hwnd, ID_RADIO_EULA_DISAGREE, BST_CHECKED );
   }

   FillRichEditControl( );

   return TRUE;
}

BOOL EulaPage::EulaPage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify )
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
         if ( g_InstallSettings.m_EulaAgreed )
         {
            PostMessage( GetParent( hwnd ), WM_NEXT_PRESSED, 0, 0 );
         }
         break;
      }

      case ID_BUTTON_CANCEL:
      {
         PostMessage( GetParent( hwnd ), WM_CANCEL_PRESSED, 0, 0 );
         break;
      }

      case ID_RADIO_EULA_AGREE:
      {
         g_InstallSettings.m_EulaAgreed = TRUE;
         EnableWindow( GetDlgItem( hwnd, ID_BUTTON_NEXT ), TRUE );
         break;
      }

      case ID_RADIO_EULA_DISAGREE:
      {
         g_InstallSettings.m_EulaAgreed = FALSE;
         EnableWindow( GetDlgItem( hwnd, ID_BUTTON_NEXT ), FALSE );
         break;
      }
   }

   return TRUE;
}

BOOL EulaPage::EulaPage_WM_NOTIFY( HWND hwnd, int idCtrl, LPNMHDR pnmh )
{
   switch( idCtrl )
   {
      case ID_RICHEDIT_EULA:
      {
         break;
      }
   }

   return TRUE;
}

BOOL EulaPage::EulaPage_WM_PAINT( HWND hwnd )
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

BOOL EulaPage::EulaPage_WM_CLOSE( HWND hwnd )
{
   Destroy( );

   return TRUE;
}

void EulaPage::FillRichEditControl( void )
{
   EulaText   eulaText   = { 0 };
   EDITSTREAM editStream = { 0 };

   // First acquire the EULA resource
   HRSRC hrSrc       = FindResource( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_RCDATA_EULA ), "RT_EULA" );
   HGLOBAL hResource = LoadResource( GetModuleHandle( NULL ), hrSrc );
  
   // get a pointer to the data
   eulaText.pEulaData = (BYTE *)LockResource( hResource );
   eulaText.pCurrPos  = eulaText.pEulaData;
   eulaText.eulaSize  = SizeofResource( GetModuleHandle( NULL ), hrSrc );

   // prepare the rich text's callback struct
   editStream.dwCookie = (DWORD)&eulaText;
   editStream.pfnCallback = (EDITSTREAMCALLBACK) EulaPage::EditStreamCallback;

   // begin filling the rich text control
   SendMessage( GetDlgItem( m_Hwnd, ID_RICHEDIT_EULA ), EM_STREAMIN, (WPARAM) SF_RTF, (LPARAM) &editStream );

   // release the EULA resource
   FreeResource( hResource );
}

DWORD CALLBACK EulaPage::EditStreamCallback( DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb )
{
   EulaText *pEulaText = (EulaText *)dwCookie;

   // determine how much we can copy; either the requested amount, or what's left of the stream
   *pcb = min( cb, (LONG) pEulaText->eulaSize ); 

   // copy it
   memcpy( pbBuff, pEulaText->pCurrPos, *pcb );

   // subtract what we read from the total size
   pEulaText->eulaSize -= *pcb;

   // increment our read position
   pEulaText->pCurrPos += *pcb;

   return 0;
}
