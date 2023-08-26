
#ifndef CORE_H_
#define CORE_H_

#include "commandBuffer.h"
#include "dialog.h"
#include "appIntegrityUserMode.h"

class Core : public ICommand
{
   public:
           
                                 Core                   ( void );
                                 ~Core                  ( );
           
           BOOL                  Initialize             ( DWORD argc, LPTSTR *argv );
           void                  UnInitialize           ( void );
           void                  Tick                   ( void );
           void                  ConvertImageToThumbnail( const char *pFullImageBuffer, uint32 fullImageSize, float aspectRatio, void **pThumbnailBuffer, uint32 *pThumbSize );
           ImageCodecInfo*       GetGDICodecByName      ( WCHAR *pEncoder );
           AppIntegrityUserMode* GetAppIntegrityUserMode( void );

   private:

           void                  GetScreenCapture       ( void );
           void                  EnumerateGDICodecs     ( void );
           
           void                  CheckScreenResForResize( void );

   virtual void                  ExecuteCommand         ( Command *pCommand );
   virtual const char *          GetCommandName         ( void ) { return "core"; }

           float                 m_ScreenCapTimer;
           
           //GDI+
           ULONG_PTR             m_GDIplusToken; //Our handle
           ImageCodecInfo       *m_pImageCodecInfo; //array of available codecs installed
           UINT                  m_NumCodecs;
           BOOL                  m_Shutdown;
           
           Dialog                m_HiddenWindow;
           AppIntegrityUserMode  m_AppIntegrity;

           HANDLE                m_RunOnceEvent;
};

#endif
