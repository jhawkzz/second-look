
#ifndef DIALOG_H_
#define DIALOG_H_

#include "systray.h"

class Dialog
{
   public:

                           Dialog ( void );
                           ~Dialog( );

          BOOL             Create ( void );
          void             Destroy( void );

          void             Update ( void );
          
          HWND             m_Hwnd;

   private:
   
   static LRESULT CALLBACK Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
   static BOOL             Dialog_WM_CREATE( HWND hwnd, LPCREATESTRUCT lpCreateStruct );
          BOOL             Dialog_WM_CREATE( HWND hwnd );
          BOOL             Dialog_WM_CLOSE( HWND hwnd );
          BOOL             Dialog_WM_SETTINGSCHANGE( HWND hwnd, WPARAM wParam, LPARAM lParam );

   static UINT             WM_TASKBARCREATED;
          Systray          m_Systray;
};

#endif
