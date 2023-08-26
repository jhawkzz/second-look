
#ifndef ABOUT_H_
#define ABOUT_H_

class About
{
   public:

                        About ( void );
                        ~About( );

               BOOL     Create( HWND parent_hwnd );

   private:

      static   LRESULT  Callback               ( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
      static   BOOL     About_WM_INITDIALOG    ( HWND hwnd, HWND hwndFocus, LPARAM lParam );
               BOOL     About_WM_INITDIALOG    ( HWND hwnd );
               BOOL     About_WM_CTLCOLORSTATIC( HWND hwnd, HDC hdc, HWND hwndChild, int type );
               BOOL     About_WM_ERASEBKGND    ( HWND hwnd, HDC hdc );
               BOOL     About_WM_MOUSEMOVE     ( HWND hwnd, int x, int y, UINT keyFlags );
               BOOL     About_WM_LBUTTONDOWN   ( HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags );
               BOOL     About_WM_LBUTTONUP     ( HWND hwnd, int x, int y, UINT keyFlags );
               BOOL     About_WM_CLOSE         ( HWND hwnd );
               BOOL     About_WM_DESTROY       ( HWND hwnd );

               HWND     m_hwnd;
               HFONT    m_hfont;
               HBITMAP  m_hBitmap;
};

#endif
