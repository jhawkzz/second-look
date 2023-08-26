
#ifndef FINISH_PAGE_H_
#define FINISH_PAGE_H_

#include "IPage.h"

class FinishPage : public IPage
{
   public:

                           FinishPage ( void );
                           ~FinishPage( );

          BOOL             Create      ( HWND parentHwnd );
          void             Destroy     ( void );

   private:
   
   static LRESULT CALLBACK Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
   static BOOL             FinishPage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam );
          BOOL             FinishPage_WM_INITDIALOG( HWND hwnd );
          BOOL             FinishPage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify );
          BOOL             FinishPage_WM_PAINT( HWND hwnd );
          BOOL             FinishPage_WM_CLOSE( HWND hwnd );

          HBITMAP          m_hBitmap;

};

#endif
