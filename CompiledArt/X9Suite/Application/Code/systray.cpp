
#include "precompiled.h"
#include "main.h"
#include "resource.h"

void Systray::Create( HWND parentHwnd )
{
   g_CommandBuffer.Register( this );

   memset( &m_Data, 0, sizeof(m_Data) );

   //create system tray icon
   m_Data.cbSize	= sizeof(m_Data);
   m_Data.hWnd		= parentHwnd;
   m_Data.uFlags	= NIF_MESSAGE | NIF_ICON | NIF_TIP;
   m_Data.hIcon	= LoadIcon( GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICON_X9) );
   m_Data.uCallbackMessage	= WM_TRAY;

   strncpy( m_Data.szTip, OptionsControl::AppName, sizeof(m_Data.szTip) - 1 );

   ActivateTrayIcon( );
}

void Systray::Destroy( void )
{
   g_CommandBuffer.Unregister( this );

   DestroyTrayIcon( );
}

BOOL Systray::ActivateTrayIcon( void )
{
   if ( g_Options.m_Options.showInSystray )
   {
      return Shell_NotifyIcon( NIM_ADD, &m_Data );
   }

   return FALSE;
}

BOOL Systray::DestroyTrayIcon( void )
{
   if ( m_Data.hWnd )
   {
      Shell_NotifyIcon( NIM_DELETE, &m_Data );
      
      DestroyWindow( m_Data.hWnd );

      return TRUE;
   }

   return FALSE;
}

void Systray::ExecuteCommand( 
   ICommand::Command *pCommand 
)
{
   const char *pString = pCommand->GetString( );

   if ( 0 == strcmpi("show", pString) )
   {
      Shell_NotifyIcon( NIM_ADD, &m_Data );
   }
   else if ( 0 == strcmpi("hide", pString) )
   {
      Shell_NotifyIcon( NIM_DELETE, &m_Data );
   }
}

BOOL Systray::Handle_WM_TRAY( HWND hwnd, int id, UINT uInputMsg )
{
   if ( WM_RBUTTONDOWN == uInputMsg )
   {        
      HMENU hmenu = CreatePopupMenu( );
         
      if ( hmenu ) 
      {
         POINT point;
         GetCursorPos( &point );

         char text[ 256 ] = { 0 };
         _snprintf( text, sizeof(text) - 1, "Open %s", OptionsControl::AppName );

         AppendMenu( hmenu, MF_STRING, Systray::ID_LAUNCHAPP, TEXT(text) );
         AppendMenu( hmenu, MF_STRING, Systray::ID_ABOUT    , TEXT("About") );
#ifdef ENABLE_DEBUGGING_FEATURES
         AppendMenu( hmenu, MF_SEPARATOR, 0, 0 );
         AppendMenu( hmenu, MF_STRING   , Systray::ID_DUMP_DATABASE, TEXT("Dump Database") );
         AppendMenu( hmenu, MF_STRING   , Systray::ID_SHUTDOWN     , TEXT("Shutdown") );
#endif

         // Force the owner of the systray icon to go to the foreground,
         // which causes the menu to be more responsive and close when clicked away.
         SetForegroundWindow( hwnd );
         
         TrackPopupMenu( 
            hmenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, 
            point.x, point.y, 0, hwnd, NULL );

         DestroyMenu( hmenu );
      }

      return TRUE;
   }
   else if ( WM_LBUTTONDBLCLK == uInputMsg )
   {
      g_Webserver.LaunchWebsite( );

      return TRUE;
   }

   return FALSE;
}

BOOL Systray::Handle_WM_COMMAND( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify )
{
   switch( id )
   {
      case ID_LAUNCHAPP:
      {
         g_Webserver.LaunchWebsite( );

         return TRUE;
      }

      case ID_ABOUT:
      {
         m_About.Create( NULL );

         return TRUE;
      }

#ifdef ENABLE_DEBUGGING_FEATURES
      case ID_DUMP_DATABASE:
      {
         g_CommandBuffer.Write( "fileExplorer dump_database" );
         return TRUE;
      }

      case ID_SHUTDOWN:
      {
         g_CommandBuffer.Write( "core shutdown" );
         return TRUE;
      }
#endif
   }

   return FALSE;
}
