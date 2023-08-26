
#include "precompiled.h"
#include "dialog.h"
#include "main.h"
#include "options.h"

UINT Dialog::WM_TASKBARCREATED;

Dialog::Dialog( void )
{
}

Dialog::~Dialog( )
{
}

BOOL Dialog::Create( void )
{
   BOOL success = FALSE;

   do
   {
      //create window for systray icon to report to
      WNDCLASSEX wndClassEx    = { 0 };
      wndClassEx.cbSize        = sizeof( WNDCLASSEX );
      wndClassEx.lpfnWndProc   = Dialog::Callback;
      wndClassEx.hInstance     = GetModuleHandle( NULL );
      wndClassEx.lpszClassName = OptionsControl::AppName;

	   RegisterClassEx( &wndClassEx );

      m_Hwnd = CreateWindow( OptionsControl::AppName, "", WS_POPUP, 0, 0, 0, 0, NULL, NULL, GetModuleHandle( NULL ), this );
      
      if ( !m_Hwnd ) break;

      success = TRUE;
   }
   while( 0 );

   return success;
}

void Dialog::Destroy( void )
{
   m_Systray.Destroy( );

   DestroyWindow( m_Hwnd );
   m_Hwnd = NULL;

   UnregisterClass( OptionsControl::AppName, GetModuleHandle( NULL ) );
}

void Dialog::Update( void )
{
   MSG msg;
   if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
   {  
      TranslateMessage( &msg );
      DispatchMessage( &msg );
   }
}

LRESULT CALLBACK Dialog::Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   Dialog *pDialog = (Dialog *)ULongToPtr( GetWindowLong( hwnd, GWLP_USERDATA ) );

   if ( pDialog || WM_CREATE == uMsg )
   {
      switch( uMsg )
      {
         HANDLE_MSG( hwnd, WM_CREATE, Dialog::Dialog_WM_CREATE );
         HANDLE_MSG( hwnd, WM_CLOSE , pDialog->Dialog_WM_CLOSE );
         
         case WM_TRAY:           return pDialog->m_Systray.Handle_WM_TRAY( hwnd, (uint32)wParam, (uint32)lParam );
         case WM_COMMAND:        return pDialog->m_Systray.Handle_WM_COMMAND( hwnd, LOWORD( wParam ), (HWND) lParam, HIWORD( wParam ) );
         case WM_SETTINGCHANGE:  return pDialog->Dialog_WM_SETTINGSCHANGE( hwnd, wParam, lParam );

         default:
         {
            // this message isn't constant, so we can't put it in a case
            if ( WM_TASKBARCREATED == uMsg )
            {
               pDialog->m_Systray.ActivateTrayIcon( );
               return TRUE;
            }
            break;
         }
      }
   }

   return DefWindowProc( hwnd, uMsg, wParam, lParam );
}

BOOL Dialog::Dialog_WM_CREATE( HWND hwnd, LPCREATESTRUCT lpCreateStruct )
{
   Dialog *pDialog = (Dialog *)lpCreateStruct->lpCreateParams;

   SetWindowLong( hwnd, GWLP_USERDATA, PtrToLong(pDialog) );

   return pDialog->Dialog_WM_CREATE( hwnd );
}

BOOL Dialog::Dialog_WM_CREATE( HWND hwnd )
{
   m_Hwnd = hwnd;
  
   m_Systray.Create( hwnd );

   // set our icon
   HICON hIcon = LoadIcon( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_ICON_X9 ) );

   SendMessage( hwnd, WM_SETICON, ICON_BIG  , (LPARAM) hIcon );
   SendMessage( hwnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon );

   WM_TASKBARCREATED = RegisterWindowMessage( TEXT("TaskbarCreated") );

   return TRUE;
}

BOOL Dialog::Dialog_WM_CLOSE( HWND hwnd )
{
   return TRUE;
}

BOOL Dialog::Dialog_WM_SETTINGSCHANGE( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
   g_CommandBuffer.Write( "core checkScreenResForResize" );
   
   return TRUE;
}
