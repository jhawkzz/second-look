
#ifndef _SYSTRAY_H_
#define _SYSTRAY_H_

#include "commandBuffer.h"
#include "about.h"

class Systray : public ICommand
{
private:
   NOTIFYICONDATA m_Data;

   static const uint32 ID_LAUNCHAPP     = 5;
   static const uint32 ID_ABOUT         = 6;
   static const uint32 ID_DUMP_DATABASE = 7;
   static const uint32 ID_SHUTDOWN      = 8;

public:
   void Create ( HWND parentHwnd );
   void Destroy( void );

   void Update( void );

   void ExecuteCommand( 
      Command *pCommand 
   );

   BOOL ActivateTrayIcon( void );
   BOOL DestroyTrayIcon( void );

   BOOL Handle_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify );
   BOOL Handle_WM_TRAY   ( HWND hwnd, int id, UINT uInputMsg );

   const char *GetCommandName( void ) { return "systray"; }

   private:
      About m_About;
};

#endif //_SYSTRAY_H_
