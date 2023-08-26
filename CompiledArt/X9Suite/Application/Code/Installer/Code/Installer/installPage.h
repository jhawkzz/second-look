
#ifndef INSTALL_PAGE_H_
#define INSTALL_PAGE_H_

#include "IPage.h"

#define MAX_INSTALL_LIST (1024)

class InstallPage : public IPage
{
   public:

      typedef enum _InstallError
      {
         InstallError_Success,
         InstallError_WriteFile,
         InstallError_RegisterDLL,
         InstallError_WriteShortcut
      }
      InstallError;


                           InstallPage ( void );
                           ~InstallPage( );

          BOOL             Create      ( HWND parentHwnd );
          void             Destroy     ( void );

   private:
   
   static LRESULT CALLBACK Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
   static BOOL             InstallPage_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam );
          BOOL             InstallPage_WM_INITDIALOG( HWND hwnd );
          BOOL             InstallPage_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify );
          BOOL             InstallPage_WM_NOTIFY( HWND hwnd, int idCtrl, LPNMHDR pnmh );
          BOOL             InstallPage_WM_PAINT( HWND hwnd );
          BOOL             InstallPage_WM_CLOSE( HWND hwnd );

          BOOL             InstallProgram( void );
          BOOL             InstallPackage( UINT id, const char *pInstallName );
          BOOL             InstallFile( UINT id, char *pInstallLogFileStr, const char *pInstallDir, const char *pInstallName );
          BOOL             CreateShortcut( int shortcutRootFolderCSIDL, char *pInstallLogFileStr, const char *pTargetDir, const char *pTargetName, const char *pShortcutSubDir, const char *pShortcutName );
          void             AddRegistryValue( RegistryValue *pRegValue );
          
          void             HandleInstallError( const char *pInstallError );
          BOOL             RollbackInstall( void );

          HBITMAP          m_hBitmap;

          //For cleanup. If we fail at any point, we'll go thru this list to clean up our mess.
          char             m_RegistryList[ MAX_INSTALL_LIST ][ MAX_PATH ];
          uint32           m_RegistryCount;

          char             m_ShortcutList[ MAX_INSTALL_LIST ][ MAX_PATH ];
          uint32           m_ShortcutCount;

          char             m_DLLRegisterList[ MAX_INSTALL_LIST ][ MAX_PATH ];
          uint32           m_DLLRegisterCount;

          char             m_InstallList[ MAX_INSTALL_LIST ][ MAX_PATH ];
          uint32           m_InstallCount;

          char             m_FolderList[ MAX_INSTALL_LIST ][ MAX_PATH ];
          uint32           m_FolderCount;

};

#endif
