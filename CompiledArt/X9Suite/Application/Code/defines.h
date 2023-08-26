
#ifndef DEFINES_H_
#define DEFINES_H_

//#define INSTALL_SERVICE   //When you want to install the service, uncomment this and run the app once. Then recomment it out.
//#define UNINSTALL_SERVICE //When you want to uninstall the service, uncomment this and run the app once. Then recomment it out.
//#define RUN_AS_SERVICE
#define SERVICE_NAME     "X9-Service"
#define SERVICE_DESC_STR "Monitors computer activity and creates a 24 hour screenshot history."

#ifdef _DEBUG
#define DEBUG_TAG " D"
#else
#define DEBUG_TAG
#endif

#define X9_VERSION      1.02f
#define X9_VERSION_STR "1.02 ("__DATE__"-"__TIME__")"DEBUG_TAG

#define HANDLE_DLGMSG( hwnd, message, fn ) \
   case (message): return ( SetDlgMsgResult( (hwnd), (message), HANDLE_##message( (hwnd), (wParam), (lParam), (fn) ) ) )

#define WM_TRAY (WM_APP + 1)

#ifdef _DEBUG
#define ENABLE_DEBUGGING_FEATURES
#endif

#endif
