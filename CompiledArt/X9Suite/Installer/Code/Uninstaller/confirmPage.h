
#ifndef CONFIRM_PAGE_H_
#define CONFIRM_PAGE_H_

#include "IPage.h"

class ConfirmPage : public IPage
{
   public:

                           ConfirmPage ( void );
                           ~ConfirmPage( );

          BOOL             Create      ( HWND parentHwnd );
          void             Destroy     ( void );

   private:
   
   static LRESULT CALLBACK Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
   static BOOL             ConfirmPage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam );
          BOOL             ConfirmPage_WM_INITDIALOG( HWND hwnd );
          BOOL             ConfirmPage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify );
          BOOL             ConfirmPage_WM_PAINT( HWND hwnd );
          BOOL             ConfirmPage_WM_CLOSE( HWND hwnd );

          BOOL             IsProgramRunning( void );

          HBITMAP          m_hBitmap;

};

#endif
