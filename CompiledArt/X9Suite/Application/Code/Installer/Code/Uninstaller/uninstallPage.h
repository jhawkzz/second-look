
#ifndef UNINSTALL_PAGE_H_
#define UNINSTALL_PAGE_H_

#include "IPage.h"

#define MAX_INSTALL_LIST (1024)

class Registry_Class;

class UninstallPage : public IPage
{
   public:

                           UninstallPage ( void );
                           ~UninstallPage( );

          BOOL             Create      ( HWND parentHwnd );
          void             Destroy     ( void );

   private:
   
   static LRESULT CALLBACK Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
   static BOOL             UninstallPage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam );
          BOOL             UninstallPage_WM_INITDIALOG( HWND hwnd );
          BOOL             UninstallPage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify );
          BOOL             UninstallPage_WM_NOTIFY( HWND hwnd, int idCtrl, LPNMHDR pnmh );
          BOOL             UninstallPage_WM_PAINT( HWND hwnd );
          BOOL             UninstallPage_WM_CLOSE( HWND hwnd );

          BOOL             UninstallProgram( void );
          BOOL             RemoveShortcut  ( int shortcutRootFolderCSIDL, const char *pShortcutFile );
          BOOL             IsFolderEmpty   ( const char *pSearchFolder );

          void             RemoveProgramShortcuts ( void );
          void             RemoveInstalledPackages( void );
          void             RemoveRegisteredDLLs   ( void );
          void             RemoveFiles            ( Registry_Class *pRegistry );
          void             RemoveDirectories      ( Registry_Class *pRegistry );
          void             RemoveRegistryKeys     ( Registry_Class *pRegistry );
          void             RemoveFileAssociations ( Registry_Class *pRegistry );
          
          HBITMAP          m_hBitmap;
};

#endif
