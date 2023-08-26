
#ifndef DIALOG_SHELL_H_
#define DIALOG_SHELL_H_

class IPage;

typedef enum _PageType
{
   PageType_Confirm,
   PageType_Uninstall,
   PageType_Finish,
   PageType_Count
}
PageType;

#define PLUTO_NAME "Pluto"

#define WM_BACK_PRESSED     (WM_APP + 1)
#define WM_NEXT_PRESSED     (WM_APP + 2)
#define WM_CANCEL_PRESSED   (WM_APP + 3)
#define WM_UNINSTALL_FAILED (WM_APP + 4)
#define WM_FINISH_PRESSED   (WM_APP + 5)

static const uint32 PLUTO_WIDTH  = 506;
static const uint32 PLUTO_HEIGHT = 392;

class DialogShell
{
   public:

                           DialogShell   ( void );
                           ~DialogShell  ( );

          BOOL             Create        ( void );
          void             Destroy       ( void );

          IPage           *GetCurrentPage( void );
          
          HWND             m_Hwnd;

   private:
   
   static LRESULT CALLBACK Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
   static BOOL             DialogShell_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam );
          BOOL             DialogShell_WM_INITDIALOG( HWND hwnd );
          BOOL             DialogShell_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify );
          BOOL             DialogShell_WM_BACK_PRESSED( HWND hwnd );
          BOOL             DialogShell_WM_NEXT_PRESSED( HWND hwnd );
          BOOL             DialogShell_WM_CANCEL_PRESSED( HWND hwnd );
          BOOL             DialogShell_WM_FINISH_PRESSED( HWND hwnd );
          BOOL             DialogShell_WM_UNINSTALL_FAILED( HWND hwnd );
          BOOL             DialogShell_WM_CLOSE( HWND hwnd );

          void             CreateWindows( void );
          void             LoadSettings( void );
          void             SaveSettings( void );

          IPage           *m_pPageList[ PageType_Count ];
          PageType         m_CurrentPage;
          BOOL             m_NoPromptToExit;

};

#endif
