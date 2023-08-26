
#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include "session.h"
#include "smtpSender.h"

class Webserver
{
protected:
   struct SSLDesc
   {
      SSL_CTX *pCtx;
   };

   struct ResourceDesc
   {
      HGLOBAL hResource;
      HRSRC   hInfo;
   };

protected:
   static const uint32 MaxConnections = 8;
   static const float  SessionTimeout;

protected:
   HashTable<const char *, ResourceDesc> m_ResourceHash;
   
   SmtpSender m_SmtpSender;
   Session    m_Sessions[ MaxConnections ];
   SOCKET     m_ListenSocket;
   SSLDesc    m_SSLDesc;
   float      m_EmailTimer;
   float      m_StartupTimer;
   char      *m_pUpdateString;   
   BOOL       m_NeedsRestart;

public:
   void Initialize( void );
   void Uninitialize( void );
   void Startup( void );
   void Update( void );

   void LaunchWebsite( void );

protected:
   void UpdateConnection(
      Session *pSocket
   );
   
   void HandleRequest(
      Session *pSocket
   );

   void SendHomePage(
      Session *pSession
   );

   void SendDayPage(
      Session *pSession
   );

   void SendLoginPage(
      Session *pSession
   );

   BOOL SendDynamicPage(
      Session *pSession
   );

   void SendServerOptionsPage(
      Session *pSession
   );

   void SendConfigurePicturesPage(
      Session *pSession
   );

   void SendConfigureEmailPage(
      Session *pSession
   );

   void SendExpiredPage(
      Session *pSession
   );

   void SendEnterKeyPage(
      Session *pSession
   );

   void SendGenerateKeyPage(
      Session *pSession
   );

   void SendDiagnosticPage(
      Session *pSession
   );

   void SendShutdownPage(
      Session *pSession
   );

   void SendConfirmClearPicturesPage(
      Session *pSession
   );

   void SendAlbumPage(
      Session *pSession
   );

   void SendThumbnail(
      Session *pSession
   );

   void SendImage(
      Session *pSession
   );

   void DoLogin(
      Session *pSession
   );

   void DoEnterKey(
      Session *pSession
   );

   void DoServerOptions(
      Session *pSession
   );

   void DoConfigureEmail(
      Session *pSession
   );

   void DoConfigurePictures(
      Session *pSession
   );

   void DoClearPictures(
      Session *pSession
   );

   void DoEmailLostPassword( 
      Session *pSession 
   );

   void DoShutdown(
      Session *pSession
   );

   void MakeHttpFormat(
      char *pHttpString,
      const char *pString,
      uint32 httpStringSize
   );

   void ParseHTTPFormat(
      char *pString,
      const char *pHttpString,
      uint32 stringSize
   );

   void BuildServerOptionsPage(
      char *pString,
      size_t size
   );

   void BuildConfigureEmailPage(
      char *pString,
      size_t size
   );

   void BuildConfigurePicturesPage(
      char *pString,
      size_t size
   );

   void SendHeader(
      Session *pSession
   );

   void SendFooter(
      Session *pSession
   );
   
   void SendResource(
      const char *pResourceName,
      Session *pSesssion
   );

   void SendHourlyEmail( void );

   int StartSSL( void );

   void ShutdownSSL( void );
   
   int GenerateSSL( void );

   int GetDaysToExpire( void );
};

#endif
