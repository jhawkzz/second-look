
#ifndef SETTINGS_PAGE_H_
#define SETTINGS_PAGE_H_

#include "IPage.h"

class SettingsPage : public IPage
{
   public:

                           SettingsPage ( void );
                           ~SettingsPage( );

          BOOL             Create      ( HWND parentHwnd );
          void             Destroy     ( void );

   private:
   
   static LRESULT CALLBACK Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
   static BOOL             SettingsPage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam );
          BOOL             SettingsPage_WM_INITDIALOG( HWND hwnd );
          BOOL             SettingsPage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify );
          BOOL             SettingsPage_WM_NOTIFY( HWND hwnd, int idCtrl, LPNMHDR pnmh );
          BOOL             SettingsPage_WM_PAINT( HWND hwnd );
          BOOL             SettingsPage_WM_CLOSE( HWND hwnd );

          BOOL             BrowseFolder( const char *pStartFolder, char *pDestFolder );
   static int CALLBACK     BrowseCallbackProc( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData );

          void             SetInstallPathDisplay( char *pPath );

          HBITMAP          m_hBitmap;

};

#endif
