
#ifndef PRECOMPILED_H_
#define PRECOMPILED_H_

// Windows 2000 or greater
#define _WIN32_WINNT 0x0500

#pragma warning(disable : 4995) //Disable security warnings for stdio functions
#pragma warning(disable : 4996) //Disable security warnings for stdio functions
#define _CRT_SECURE_NO_DEPRECATE //Disable security warnings for stdio functions

#include <crtdbg.h>
#include <fcntl.h>
#include <float.h>
#include <io.h>
#include <math.h>
#include <stdio.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <time.h>
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>

#include <shlobj.h>
#include <psapi.h>
#include <Aclapi.h>

#include <gdiplus.h>
using namespace Gdiplus;

#include "timer.h"
#include "types.h"
#include "defines.h"

#include "resource.h"

//open ssl
#include "..\SDKs\openssl\include\openssl\rsa.h"
#include "..\SDKs\openssl\include\openssl\crypto.h"
#include "..\SDKs\openssl\include\openssl\x509.h"
#include "..\SDKs\openssl\include\openssl\pem.h"
#include "..\SDKs\openssl\include\openssl\ssl.h"
#include "..\SDKs\openssl\include\openssl\err.h"


#pragma comment ( lib, "Ws2_32" )
#pragma comment ( lib, "Rpcrt4" )

#ifdef _DEBUG
   #pragma comment ( lib, "..\\SDKs\\OpenSSL\\lib\\ssleay32MTd" )
   #pragma comment ( lib, "..\\SDKs\\OpenSSL\\lib\\libeay32MTd" )
#else
   #pragma comment ( lib, "..\\SDKs\\OpenSSL\\lib\\ssleay32MT" )
   #pragma comment ( lib, "..\\SDKs\\OpenSSL\\lib\\libeay32MT" )
#endif

#endif
