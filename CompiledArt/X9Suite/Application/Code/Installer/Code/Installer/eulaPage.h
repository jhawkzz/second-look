
#ifndef EULA_PAGE_H_
#define EULA_PAGE_H_

#include "IPage.h"

class EulaPage : public IPage
{
   typedef struct _EulaText
   {
      BYTE * pEulaData;
      BYTE * pCurrPos;
      DWORD  eulaSize;
   }
   EulaText;


   public:

                           EulaPage ( void );
                           ~EulaPage( );

          BOOL             Create   ( HWND parentHwnd );
          void             Destroy  ( void );

   private:
   
   static LRESULT CALLBACK Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
   static BOOL             EulaPage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam );
          BOOL             EulaPage_WM_INITDIALOG( HWND hwnd );
          BOOL             EulaPage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify );
          BOOL             EulaPage_WM_NOTIFY( HWND hwnd, int idCtrl, LPNMHDR pnmh );
          BOOL             EulaPage_WM_PAINT( HWND hwnd );
          BOOL             EulaPage_WM_CLOSE( HWND hwnd );
   static DWORD   CALLBACK EditStreamCallback( DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb );

          void             FillRichEditControl( void );

          HBITMAP          m_hBitmap;

};

#endif
