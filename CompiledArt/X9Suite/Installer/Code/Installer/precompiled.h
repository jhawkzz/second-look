
#ifndef PRECOMPILED_H_
#define PRECOMPILED_H_

// win98 onleh
#define _WIN32_WINDOWS 0x0410
#define WINVER         0x0410


#pragma warning(disable : 4995) //Disable security warnings for stdio functions
#pragma warning(disable : 4996) //Disable security warnings for stdio functions
#define _CRT_SECURE_NO_DEPRECATE //Disable security warnings for stdio functions

#define HANDLE_DLGMSG( hwnd, message, fn ) \
   case (message): return ( SetDlgMsgResult( (hwnd), (message), HANDLE_##message( (hwnd), (wParam), (lParam), (fn) ) ) )

// includes
#include <crtdbg.h>
#include <io.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <shlobj.h>
#include <windowsx.h>

#include "resource.h"

#include "..\\Shared\\timer.h"
#include "..\\Shared\\types.h"
#include "..\\Shared\\registry_class.h"

#include "winmain.h"

#endif
