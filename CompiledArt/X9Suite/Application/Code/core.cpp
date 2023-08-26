
#include "precompiled.h"
#include "core.h"
#include "main.h"
#include "serviceInit.h"
#include "imageStream.h"
#include <Psapi.h>

Core::Core( void )
{
   m_RunOnceEvent    = NULL;
   m_pImageCodecInfo = NULL;
}

Core::~Core( )
{
}

LONG WINAPI UnhandledExceptions(
   struct _EXCEPTION_POINTERS *pInfo
)
{
   MessageBox( GetForegroundWindow( ), "We are very sorry, "APP_NAME" encountered a serious error and had to close.  Please contact "COMPANY_NAME" if you need assistance.", APP_NAME, MB_ICONINFORMATION | MB_OK );

   if ( pInfo )
   {
      g_Log.Write( Log::TypeError, "Unhandled Exception: Code 0x%x Address: 0x%x", pInfo->ExceptionRecord->ExceptionCode, pInfo->ExceptionRecord->ExceptionAddress );
   }
   else
   {
      g_Log.Write( Log::TypeError, "Unhandled Exception: Exception Info Unavailable" );
   }

   return EXCEPTION_EXECUTE_HANDLER;
}

BOOL Core::Initialize( DWORD argc, LPTSTR *argv )
{
   //Watch for memory leaks in DEBUG only
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF );
	//_CrtSetBreakAlloc( 2301 );
   //End leak check

   SetUnhandledExceptionFilter( UnhandledExceptions );

   // test for an existing instance of X9.
   m_RunOnceEvent = CreateEvent( NULL, TRUE, TRUE, OptionsControl::RunOnceMutex );
   DWORD error    = GetLastError( );

   // if we found it, we won't run, but we'll take ownership so the original
   // can launch a browser
	if ( ERROR_ALREADY_EXISTS == error )
   {
      // reset the event so the real X9 knows to launch a browser
      ResetEvent( m_RunOnceEvent );

      return FALSE;
   }

   // Initialize GDI+.
   GdiplusStartupInput gdiplusStartupInput;
   GdiplusStartup( &m_GDIplusToken, &gdiplusStartupInput, NULL );

   EnumerateGDICodecs( );

   Timer::Create( );

   g_CommandBuffer.Register( &g_Log );
   g_CommandBuffer.Register( this );

   m_AppIntegrity.EnableIntegrity( );

   g_Options.Create( );

   g_Log.Initialize( g_Options.m_Options.databasePath );

   g_Webserver.Initialize( );

   g_FileExplorer.Create( );

   if ( !m_HiddenWindow.Create( ) )
   {
      g_Log.Write( Log::TypeError, "Core: Could not create a window" );
      return FALSE;
   }

   if ( !g_FileExplorer.OpenDatabase( ) )
   {
      g_Log.Write( Log::TypeError, "Core: Could not open database" );
      return FALSE;
   }

   CheckScreenResForResize( );

   m_ScreenCapTimer = 0.00f;

   return TRUE;
}

void Core::UnInitialize( void )
{
   m_HiddenWindow.Destroy( );

   g_FileExplorer.CloseDatabase( );

   g_Webserver.Uninitialize( );

   g_Log.Uninitialize( );

   // disables monitoring
   m_AppIntegrity.DisableIntegrity( );

   delete [] m_pImageCodecInfo;

   // shut down GDI
   GdiplusShutdown( m_GDIplusToken );

   // kill our event
   CloseHandle( m_RunOnceEvent );

   SetUnhandledExceptionFilter( NULL );
}

void Core::Tick( void )
{
   m_Shutdown = FALSE;

   while( FALSE == m_Shutdown )
   {
      #ifdef RUN_AS_SERVICE
         // As a service, we need to see if we've been asked to stop.
         if ( ServiceInit::ServiceCheckForShutdown( ) )
         {
            return;
         }
      #endif

      // test our run once event. If it's not signaled, we should launch a browser and signal it
      DWORD waitResult = WaitForSingleObject( m_RunOnceEvent, 0 );
      if ( waitResult != WAIT_OBJECT_0 )
      {
         // set the event again
         SetEvent( m_RunOnceEvent );

         // launch our browser
         g_Webserver.LaunchWebsite( );
      }

      m_HiddenWindow.Update( );

      // first update our timer
      Timer::Update( );

      g_CommandBuffer.FlushCommands( );

      g_Webserver.Update( );

      SYSTEMTIME systemTime;
      GetLocalTime( &systemTime );

      // if the current day and hour is valid, allow screen capturing
      if ( g_Options.m_Options.days[ systemTime.wDayOfWeek ].periods[ systemTime.wHour / 6 ] )
      {
         if ( m_ScreenCapTimer < Timer::m_total && g_Options.m_Options.picsPerHour )
         {
            // take our next screenshot
            GetScreenCapture( );

            // convert to seconds per picture
            uint32 picsPerMinute = 60 / g_Options.m_Options.picsPerHour;
            uint32 secondsPerPic = picsPerMinute * 60;

            m_ScreenCapTimer = Timer::m_total + secondsPerPic;
         }   
      }

      g_Options.Update( );

      // if this returns false, we had a corrupt database and couldn't repair. We have to shut down
      if ( !g_FileExplorer.CheckForCorruptDatabase( ) )
      {
         MessageBox( GetForegroundWindow( ), "We are very sorry. "APP_NAME" is unable to generate a database and cannot run.", APP_NAME, MB_ICONINFORMATION | MB_OK );
         m_Shutdown = TRUE;
      }

      Sleep( 1 );
   }
}

void Core::ConvertImageToThumbnail( const char *pFullImageBuffer, uint32 fullImageSize, float aspectRatio, void **pThumbnailBuffer, uint32 *pThumbSize )
{
   // now put it in an image stream.
   ImageStream imageStreamBmp;
   imageStreamBmp.Write( pFullImageBuffer, fullImageSize, NULL );

   Image image( &imageStreamBmp );
   Image *pThumbImage = image.GetThumbnailImage( DEF_THUMB_WIDTH, (uint32) ((float)DEF_THUMB_WIDTH / aspectRatio) );

   // get the JPEG codec
   ImageCodecInfo *pCodecInfo = g_Core.GetGDICodecByName( L"image/jpeg" );
   _ASSERTE( pCodecInfo );

   // now convert the image to a jpg and save it to our thumbnail stream
   ImageStream thumbnailImage;
   pThumbImage->Save( &thumbnailImage, &pCodecInfo->Clsid, NULL );

   delete pThumbImage;

   // get the size of the stream
   STATSTG statStg = { 0 };
   thumbnailImage.Stat( &statStg, STATFLAG_NONAME );

   // seek to the beginning of the stream
   LARGE_INTEGER largeInt = { 0 };
   thumbnailImage.Seek( largeInt, SEEK_SET, NULL );

   *pThumbSize = (sint32) statStg.cbSize.QuadPart;

   // allocate a buffer at the provided address and copy the thumbnail in.
   *pThumbnailBuffer = new char[ (sint32) statStg.cbSize.QuadPart ];
   thumbnailImage.Read( *pThumbnailBuffer, (sint32) statStg.cbSize.QuadPart, NULL );
}

void Core::GetScreenCapture( void )
{
   // first we need the desktop window handle
   HWND desktopHwnd  = NULL;
   HDC desktopDC     = NULL;
   HDC bmpDC         = NULL;
   HBITMAP hBitmap   = NULL;
   HGDIOBJ oldObject = NULL;
   
   do
   {
      desktopHwnd = GetDesktopWindow( );
      if ( !desktopHwnd )
      {
         g_Log.Write( Log::TypeError, "Error: %s line %d - Failed to get desktop HWND. Last Error: %d", __FUNCTION__, __LINE__, GetLastError( ) );
         break;
      }

      // next we need it's device context, which holds its graphics buffer
      desktopDC = GetDC( desktopHwnd );
      if ( !desktopDC )
      {
         g_Log.Write( Log::TypeError, "Error: %s line %d - Failed to get desktop DC. Last Error: %d", __FUNCTION__, __LINE__, GetLastError( ) );
         break;
      }


      // now we need to create a bitmap so we have access to the data
      
      // first, get the dimensions of the screen. We ask for virtual screen because the desktop could span multiple monitors
      uint32 xScreenSize = GetSystemMetrics( SM_CXVIRTUALSCREEN );
      uint32 yScreenSize = GetSystemMetrics( SM_CYVIRTUALSCREEN );

      if ( !xScreenSize || !yScreenSize )
      {
         g_Log.Write( Log::TypeError, "Error: %s line %d - 0 screen size. Last Error: %d", __FUNCTION__, __LINE__, GetLastError( ) );
         break;
      }

      // now let's create that bitmap.
      BITMAPINFO bmpInfo = { 0 };
      bmpInfo.bmiHeader.biBitCount  = 32;
      bmpInfo.bmiHeader.biHeight    = yScreenSize;
      bmpInfo.bmiHeader.biWidth     = xScreenSize;
      bmpInfo.bmiHeader.biPlanes    = 1;
      bmpInfo.bmiHeader.biSize      = sizeof( bmpInfo.bmiHeader );
      bmpInfo.bmiHeader.biSizeImage = xScreenSize * (bmpInfo.bmiHeader.biBitCount / 8) * yScreenSize;

      // create the buffer for storing the bitmap
      uint32 *pBmpBits = NULL;
      bmpDC            = CreateCompatibleDC( desktopDC );
      if ( !bmpDC )
      {
         g_Log.Write( Log::TypeError, "Error: %s line %d - Failed to create DC. Last Error: %d", __FUNCTION__, __LINE__, GetLastError( ) );
         break;
      }

      hBitmap = CreateDIBSection( bmpDC, &bmpInfo, DIB_RGB_COLORS, (void **)&pBmpBits, NULL, 0 );
      if ( !hBitmap )
      {
         g_Log.Write( Log::TypeError, "Error: %s line %d - Failed to create bitmap. Last Error: %d", __FUNCTION__, __LINE__, GetLastError( ) );
         break;
      }

      oldObject = SelectObject( bmpDC, hBitmap );
      if ( !oldObject || HGDI_ERROR == oldObject )
      {
         g_Log.Write( Log::TypeError, "Error: %s line %d - Failed to Select Object. Last Error: %d", __FUNCTION__, __LINE__, GetLastError( ) );
         break;
      }

      if ( !desktopDC || !bmpDC )
      {
         g_Log.Write( Log::TypeError, "Error: %s line %d - DesktopDC or BmpDC lost. Last Error: %d", __FUNCTION__, __LINE__, 0 );
         break;
      }

      // now we have a buffer big enough to hold the screenshot. Let's copy it in.
      if ( !BitBlt( bmpDC, 0, 0, xScreenSize, yScreenSize, desktopDC, 0, 0, SRCCOPY ) )
      {
         g_Log.Write( Log::TypeError, "Error: %s line %d - Failed to BitBlt. Last Error: %d", __FUNCTION__, __LINE__, GetLastError( ) );
         break;
      }
        
      // setup the bitmap's file header
      BITMAPFILEHEADER bmpFileHeader = { 0 };
      bmpFileHeader.bfType    = 0x4D42;
      bmpFileHeader.bfSize    = sizeof( bmpFileHeader ) + sizeof( bmpInfo.bmiHeader ) + bmpInfo.bmiHeader.biSizeImage;
      bmpFileHeader.bfOffBits = sizeof( bmpFileHeader ) + sizeof( bmpInfo.bmiHeader );

      // now put the bitmap into an image stream
      ImageStream imageStreamBmp;
      imageStreamBmp.Write( &bmpFileHeader    , sizeof( bmpFileHeader )      , NULL );
      imageStreamBmp.Write( &bmpInfo.bmiHeader, sizeof( bmpInfo.bmiHeader )  , NULL );
      imageStreamBmp.Write( pBmpBits          , bmpInfo.bmiHeader.biSizeImage, NULL );

      // create a GDI+ image based on this stream, which is the bitmap
      Image image( &imageStreamBmp, TRUE );

      float imageAspect = (float) xScreenSize / (float) yScreenSize;

      // if they've chosen full, use the real width of the picture
      float imageWidth;
      if ( ScreenCapSize_Full == g_Options.m_Options.screenCapSize )
      {
         imageWidth = (float) xScreenSize;
      }
      else
      {
         // otherwise take the width we define for their desired size.
         imageWidth = g_Options.sScreenCapSize[ g_Options.m_Options.screenCapSize ];
      }

      float imageHeight  = imageWidth / imageAspect;
      Image *pThumbImage = image.GetThumbnailImage( (sint32) imageWidth, (sint32) imageHeight );
      if ( !pThumbImage )
      {
         break;
      }

      // get the JPEG codec
      ImageCodecInfo *pCodecInfo = GetGDICodecByName( L"image/jpeg" );
      _ASSERTE( pCodecInfo );

      // now convert the image to a jpg and save it to our imageStreamOut
      ImageStream imageStreamJpeg;

      // JPEG compression is 0 - 100 (0 is the most compression and 100 is the least compression)
      ULONG compressionRate = 100;

      EncoderParameters encoderParameters;
      encoderParameters.Count = 1;
      encoderParameters.Parameter[ 0 ].Guid = EncoderQuality;
      encoderParameters.Parameter[ 0 ].Type = EncoderParameterValueTypeLong;
      encoderParameters.Parameter[ 0 ].NumberOfValues = 1;
      encoderParameters.Parameter[ 0 ].Value = &compressionRate;
      
      // if we find a valid file size, or actually hit 0 on our compression rate and STILL can't, just abort.
      BOOL fileSizeSafe  = FALSE;
      BOOL statGetFailed = FALSE;
      while( !fileSizeSafe && compressionRate > 0 )
      {
         if ( pThumbImage->Save( &imageStreamJpeg, &pCodecInfo->Clsid, &encoderParameters ) != Ok )
         {
            break;
         }

          // get the size of the stream, because we need to guarantee it's small enough for the DB
         STATSTG statStg = { 0 };
         if ( imageStreamJpeg.Stat( &statStg, STATFLAG_NONAME ) != S_OK ) { statGetFailed = TRUE; break; }

         if ( (sint32) statStg.cbSize.QuadPart < g_FileExplorer.GetMaxFileSize( ) )
         {
            fileSizeSafe = TRUE;
         }
         else
         {
            // reset the stream so we can write to it again.
            ULARGE_INTEGER newSize = { 0 };
            imageStreamJpeg.SetSize( newSize );

            // since the file was too large, let's increase the compression until it isn't
            compressionRate -= 5;
         }
      }

      // clean up the thumbnail image
      delete pThumbImage;

      if ( statGetFailed )
      {
         g_Log.Write( Log::TypeError, "Error: %s line %d - Failed pStream->Stat. Last Error: %d", __FUNCTION__, __LINE__, GetLastError( ) );
         break;
      }

      // add the jpeg to our database
      if ( !g_FileExplorer.AddStream( &imageStreamJpeg, (sint16) imageWidth, (sint16) imageHeight ) )
      {
         g_Log.Write( Log::TypeWarning, "Warning: %s line %d - Could not take screen capture.", __FUNCTION__, __LINE__ );
      }
   }
   while( 0 );

   // clean up each of the objects
   if ( bmpDC )
   {
      SelectObject( bmpDC, oldObject );
      DeleteDC( bmpDC );
   }

   if ( hBitmap )
   {
      DeleteObject( hBitmap );
   }
   
   if ( desktopDC )
   {
      ReleaseDC( desktopHwnd, desktopDC );
   }
}

ImageCodecInfo* Core::GetGDICodecByName( WCHAR *pEncoder )
{
   uint32 i;
   for ( i = 0; i < m_NumCodecs; i++ )
   {
      if ( !wcscmp( m_pImageCodecInfo[ i ].MimeType, pEncoder ) )
      {
         return &m_pImageCodecInfo[ i ];
      }
   }

   return NULL;
}

void Core::EnumerateGDICodecs( void )
{
   UINT size = 0;

   // find out the number of codecs and size required to allocate an array for em
   GetImageEncodersSize( &m_NumCodecs, &size );

   // allocate a buffer for the codecs
   m_pImageCodecInfo = new ImageCodecInfo[ size ];

   // get our list of codecs
   GetImageEncoders( m_NumCodecs, size, m_pImageCodecInfo );
}

AppIntegrityUserMode* Core::GetAppIntegrityUserMode( void )
{
   return &m_AppIntegrity;
}

void Core::ExecuteCommand( 
   ICommand::Command *pCommand 
)
{
   const char *pString = pCommand->GetString( );

   if ( 0 == strcmpi(pString, "shutdown") )
   {
      m_Shutdown = TRUE;
   }
   else if ( 0 == strcmpi(pString, "checkScreenResForResize") )
   {
      CheckScreenResForResize( );
   }
   else if ( 0 == strcmpi(pString, "uninstall") )
   {
   }
}

void Core::CheckScreenResForResize( void )
{
   // all we care to do is update the resolution we store.
   uint32 xScreenSize = GetSystemMetrics( SM_CXVIRTUALSCREEN );
   uint32 yScreenSize = GetSystemMetrics( SM_CYVIRTUALSCREEN );

   // because the screen resolution is now bigger, we cannot assume the database can store
   // the images, so resize the database accordingly.
   if ( xScreenSize > g_Options.m_Options.xScreenSize || yScreenSize > g_Options.m_Options.yScreenSize )
   {
      g_Log.Write( Log::TypeWarning, "Warning: %s line %d - Screen resolution larger. Old X: %d Old Y: %d New X: %d New Y: %d", __FUNCTION__, 
                                                                                                                                __LINE__, 
                                                                                                                                g_Options.m_Options.xScreenSize, 
                                                                                                                                g_Options.m_Options.yScreenSize, 
                                                                                                                                xScreenSize, 
                                                                                                                                yScreenSize );
      g_FileExplorer.ResizeDatabase( );

      // store the new dimensions of the screen
      g_Options.m_Options.xScreenSize = xScreenSize;
      g_Options.m_Options.yScreenSize = yScreenSize;

      // flag the options dirty so we update it
      g_Options.m_IsDirty = TRUE;
   }
}
