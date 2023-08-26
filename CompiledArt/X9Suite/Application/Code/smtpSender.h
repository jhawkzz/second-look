
#ifndef SMTPSENDER_H_
#define SMTPSENDER_H_

#include "ThreadLock.h"

class SmtpSender
{
public:
   enum AttachmentType
   {
      AttachmentType_Jpeg,
   };

   static const char *AttachmentTypeToString(
      AttachmentType type
   )
   {
      if ( AttachmentType_Jpeg == type ) return "image/jpeg";
      
      _ASSERTE( FALSE && "Unrecognized attachment type" );
      return "unknown";
   }

protected:
   struct Attachement
   {
      size_t size;
      AttachmentType type;
      void *pData;
      char name[ 256 ];
   };

protected:
   static const uint32 MaxAttachments = 32;

protected:
   Attachement m_Attachments[ MaxAttachments ];
   Lock   m_Lock;

   uint32 m_NumAttachments;   
   HANDLE m_hThread;
   BOOL   m_CancelSend;
   BOOL   m_RunThread;
   char  *m_pSubject;
   char  *m_pMessage;
   char   m_SendTo[ 256 ];
   char   m_SendFrom[ 256 ];
   char   m_Password[ 256 ];

public:
   void Create( void );

   void Destroy( void );

   void SendTo(
      const char *pEmail
   );

   void SendFrom( 
      const char *pEmail
   );
   
   void Subject(
      const char *pSubject,
      ...
   );

   void Message(
      const char *pMessage,
      ...
   );

   void Password(
      const char *pPassword
   );

   void AddAttachment(
      const char *pName,
      AttachmentType type,
      void *pAttachement,
      size_t size
   );
   
   void Send( void );
   
   void Stop( void );

protected:
   const char *GetSSLDomainName(
      const char *pEmail
   );

   void RunThread( void );

protected:
   static DWORD WINAPI ThreadProc(
      void *pParam
   );
};

#endif //SMTPSENDER_H_
