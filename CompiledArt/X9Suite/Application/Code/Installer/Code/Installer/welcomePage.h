
#ifndef WELCOME_PAGE_H_
#define WELCOME_PAGE_H_

#include "IPage.h"

class WelcomePage : public IPage
{
   public:

                           WelcomePage ( void );
                           ~WelcomePage( );

          BOOL             Create      ( HWND parentHwnd );
          void             Destroy     ( void );

   private:
   
   static LRESULT CALLBACK Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
   static BOOL             WelcomePage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam );
          BOOL             WelcomePage_WM_INITDIALOG( HWND hwnd );
          BOOL             WelcomePage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify );
          BOOL             WelcomePage_WM_PAINT( HWND hwnd );
          BOOL             WelcomePage_WM_CLOSE( HWND hwnd );

          BOOL             IsProgramRunning( void );

          HBITMAP          m_hBitmap;

};

#endif
