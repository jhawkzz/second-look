
#include "precompiled.h"
#include "main.h"
#include "smtpSender.h"
#include "secureSocket.h"
#include "base64.h"

void SmtpSender::Create( void ) 
{
   m_SendTo[ 0 ] = 0;
   m_SendFrom[ 0 ] = 0;
   m_Password[ 0 ] = 0;
   m_pSubject = NULL;
   m_pMessage = NULL;
   m_NumAttachments = 0;
   m_CancelSend = FALSE;
   m_RunThread = TRUE;

   DWORD threadId;
   m_hThread = CreateThread( NULL, 1024, ThreadProc, (void *) this, CREATE_SUSPENDED, &threadId );
}

   
void SmtpSender::Destroy( void )
{
   uint32 i;

   //cancel our thread while loop
   m_RunThread = FALSE;

   //stop the thread if it is currently running
   Stop( );

   CloseHandle( m_hThread );

   delete [] m_pSubject;
   delete [] m_pMessage;

   for ( i = 0; i < m_NumAttachments; i++ )
   {
      free( m_Attachments[ i ].pData );
   }
}

void SmtpSender::SendTo(
   const char *pEmail
)
{
   strncpy( m_SendTo, pEmail, sizeof(m_SendTo) - 1 );
   m_SendTo[ sizeof(m_SendTo) - 1 ] = NULL;
}

void SmtpSender::SendFrom( 
   const char *pEmail
)
{
   strncpy( m_SendFrom, pEmail, sizeof(m_SendFrom) - 1 );
   m_SendFrom[ sizeof(m_SendFrom) - 1 ] = NULL;
}

void SmtpSender::Subject(
   const char *pSubject,
   ...
)
{
   va_list marker;

   size_t length = strlen( pSubject ) * 2;

   m_pSubject = new char[ length + 1 ];
   
   va_start( marker, pSubject );

   _vsnprintf( m_pSubject, length, pSubject, marker );
   m_pSubject[ length ] = NULL;

   va_end( marker );
}

void SmtpSender::Message(
   const char *pMessage,
   ...
)
{
   va_list marker;

   size_t length = strlen( pMessage ) * 2;

   m_pMessage = new char[ length + 1 ];
   
   va_start( marker, pMessage );

   _vsnprintf( m_pMessage, length, pMessage, marker );
   m_pMessage[ length ] = NULL;

   va_end( marker );
}

void SmtpSender::Password(
   const char *pPassword
)
{
   strncpy( m_Password, pPassword, sizeof(m_Password) - 1 );
   m_Password[ sizeof(m_Password) - 1 ] = NULL;
}

void SmtpSender::AddAttachment(
   const char *pName,
   AttachmentType type,
   void *pAttachment,
   size_t size
)
{
   if ( MaxAttachments == m_NumAttachments ) return;

   m_Attachments[ m_NumAttachments ].type = type;
   m_Attachments[ m_NumAttachments ].size = size;

   m_Attachments[ m_NumAttachments ].pData = malloc( size );
   memcpy( m_Attachments[ m_NumAttachments ].pData, pAttachment, size );
   
   strncpy( m_Attachments[ m_NumAttachments ].name, pName, sizeof(m_Attachments[ m_NumAttachments ].name) - 1 );
   m_Attachments[ m_NumAttachments ].name[ sizeof(m_Attachments[ m_NumAttachments ].name) - 1 ] = NULL;

   ++m_NumAttachments;
}

void SmtpSender::Send( void )
{
   ResumeThread( m_hThread );
}

void SmtpSender::Stop( void )
{
   BOOL isSending = ThreadLock::Acquire( m_Lock, FALSE );

   //it can't acquire a lock, which means the other thread is sending
   if ( FALSE == isSending )
   {
      //print a message so the user knows the previous email didn't send
      g_Log.Write( Log::TypeWarning, "Email: Outgoing email was stopped before it finished sending." );

      m_CancelSend = TRUE;

      //wait for it to cancel
      ThreadLock lock( m_Lock );

      m_CancelSend = FALSE;
   }
   else
   {
      //it did acquire the lock, which means
      //the other thread is not sending
      ThreadLock::Release( m_Lock );
   }
}

const char *SmtpSender::GetSSLDomainName(
   const char *pEmail
)
{
   const char *pAt = strrchr( pEmail, '@' );

   if ( NULL == pAt )
   {
      g_Log.Write( Log::TypeError, "Email: Invalid Address" );
   }
   else
   {
      ++pAt;

      if ( 0 == stricmp(pAt, "gmail.com") )
      {
         return "smtp.gmail.com";
      }

      g_Log.Write( Log::TypeError, "Email: There is no known secure smtp server for email client %s", pAt );
   }

   return NULL;
}

void SmtpSender::RunThread( void )
{
   while ( m_RunThread )
   {
      {
         ThreadLock lock( m_Lock );

         SecureSocket smtpSocket;
         SSL_CTX *pCtx = NULL;
         uint32 i;

         do
         {
            const char *pDomain = GetSSLDomainName( m_SendTo );
            if ( NULL == pDomain ) break;

            hostent *pHost = gethostbyname( pDomain );
            if ( NULL == pHost )
            {
               g_Log.Write( Log::TypeError, "Email: Could not find DNS Lookup for the secure smtp %s", pDomain );
               break;
            }

            SOCKET connectionSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
                  
            char *pIp = inet_ntoa (*(struct in_addr *)*pHost->h_addr_list);

            // Set up the sockaddr structure
            SOCKADDR_IN service;
            service.sin_family = AF_INET;
            service.sin_addr.s_addr = inet_addr(pIp);
            service.sin_port = htons(465);

            if ( TRUE == m_CancelSend ) break;

            int result = connect( connectionSocket, (sockaddr *) &service, sizeof(service) );
            
            if ( SOCKET_ERROR == result )
            {
               closesocket( connectionSocket );
               g_Log.Write( Log::TypeError, "Email: Could not connect to the email server's IP %s, are you sure it offers secure SSL email?", pIp );
               break;
            }

            //set to non blocking
            u_long nonBlocking = 1;
            ioctlsocket( connectionSocket, FIONBIO, &nonBlocking );

            SSL_METHOD *pMethod;

            if ( TRUE == m_CancelSend ) break;

            //start up ctx
            pMethod = SSLv23_client_method( );

            if ( NULL == pMethod )
            {
               g_Log.Write( Log::TypeError, "Email: Could not create a secure method" );
               break;
            }

            pCtx = SSL_CTX_new( pMethod );

            if ( NULL == pCtx )
            {
               g_Log.Write( Log::TypeError, "Email: Could not create a secure ctx" );
               break;
            }
            

            SSL *pSSL = SSL_new( pCtx );

            if ( NULL == pSSL )
            {
               g_Log.Write( Log::TypeError, "Email: Could not create an SSL" );
               break;
            }
            
            smtpSocket.Create( connectionSocket, &service );
            smtpSocket.SetSSL( pSSL );

            do
            {
               smtpSocket.SetSSLClient( );

               if ( TRUE == m_CancelSend ) break;
            }
            while ( TRUE == smtpSocket.GetNeedsHandshake( ) );

            if ( TRUE == m_CancelSend ) break;

            char buffer[ 1024 ];
            char encoded[ 1024 ];

            //receive server name
            while ( FALSE == smtpSocket.Read( ) )
            {
               if ( TRUE == m_CancelSend ) break;
            }

            if ( TRUE == m_CancelSend ) break;

            const char *pString = "HELO ";
            smtpSocket.Write( pString, strlen(pString) );

            pString = OptionsControl::CompanyDomain;
            smtpSocket.Write( pString, strlen(pString) );

            pString = "\r\n";
            smtpSocket.WriteImmediate( pString, strlen(pString) );

            //receive welcome message
            while ( FALSE == smtpSocket.Read( ) )
            {
               if ( TRUE == m_CancelSend ) break;
            }

            if ( TRUE == m_CancelSend ) break;

            pString = "AUTH login\r\n";
            smtpSocket.WriteImmediate( pString, strlen(pString) );

            smtpSocket.ReadClear( );

            while ( 1 )
            {
               //receive "Username:"
               smtpSocket.Read( );
               
               if ( smtpSocket.GetReadSize( ) )
               {
                  smtpSocket.TerminateReadBuffer( );
                  
                  char *pEncoded = ((char *) smtpSocket.GetReadBuffer( )) + 4;
                  b64decode( buffer, sizeof(buffer), pEncoded, strlen(pEncoded) );

                  if ( 0 == stricmp(buffer, "Username:") )
                  {
                     break;
                  }
               }

               if ( TRUE == m_CancelSend ) break;
            }

            if ( TRUE == m_CancelSend ) break;

            b64encode( encoded, sizeof(encoded), m_SendTo, strlen(m_SendTo) );
            smtpSocket.Write( encoded, strlen(encoded) );

            if ( TRUE == m_CancelSend ) break;

            pString = "\r\n";
            smtpSocket.WriteImmediate( pString, strlen(pString) );

            smtpSocket.ReadClear( );

            while ( 1 )
            {
               //receive "Password:"
               smtpSocket.Read( );

               if ( smtpSocket.GetReadSize( ) )
               {
                  smtpSocket.TerminateReadBuffer( );
                  
                  char *pEncoded = ((char *) smtpSocket.GetReadBuffer( )) + 4;
                  b64decode( buffer, sizeof(buffer), pEncoded, strlen(pEncoded) );

                  if ( 0 == stricmp(buffer, "Password:") )
                  {
                     break;
                  }
               }

               if ( TRUE == m_CancelSend ) break;
            }

            if ( TRUE == m_CancelSend ) break;

            //send password
            b64encode( encoded, sizeof(encoded), m_Password, strlen(m_Password) );
            smtpSocket.Write( encoded, strlen(encoded) );

            pString = "\r\n";
            smtpSocket.WriteImmediate( pString, strlen(pString) );

            if ( TRUE == m_CancelSend ) break;

            smtpSocket.ReadClear( );

            BOOL hasError = FALSE;

            while ( 1 )
            {
               //receive "Accepted or Rejected"
               smtpSocket.Read( );

               if ( smtpSocket.GetReadSize( ) )
               {
                  smtpSocket.TerminateReadBuffer( );

                  if ( strstr((char *)smtpSocket.GetReadBuffer( ), "Accepted") )
                  {
                     break;
                  }
                  else 
                  {
                     g_Log.Write( Log::TypeError, "Email login error: %s", smtpSocket.GetReadBuffer( ) );
                     hasError = TRUE;
                     break;
                  }
               }

               if ( TRUE == m_CancelSend ) break;
            }

            if ( TRUE == m_CancelSend ) break;
            if ( TRUE == hasError ) break;
            
            _snprintf( buffer, sizeof(buffer) - 1, "MAIL FROM:<%s>\r\n", m_SendFrom );
            smtpSocket.WriteImmediate( buffer, strlen(buffer) );

            //receive "ok"
            while ( FALSE == smtpSocket.Read( ) )
            {
               if ( TRUE == m_CancelSend ) break;
            }

            if ( TRUE == m_CancelSend ) break;
            
            _snprintf( buffer, sizeof(buffer) - 1, "RCPT TO:<%s>\r\n", m_SendTo );
            smtpSocket.WriteImmediate( buffer, strlen(buffer) );

            //receive "ok"
            while ( FALSE == smtpSocket.Read( ) )
            {
               if ( TRUE == m_CancelSend ) break;
            }

            if ( TRUE == m_CancelSend ) break;

            pString = "DATA\r\n";
            smtpSocket.WriteImmediate( pString, strlen(pString) );

            //receive "ok, following w/ how to close the message: \r\n.\r\n"
            while ( FALSE == smtpSocket.Read( ) )
            {
               if ( TRUE == m_CancelSend ) break;
            }

            if ( TRUE == m_CancelSend ) break;

            _snprintf( buffer, sizeof(buffer) - 1, "From: \"%s\" <%s>\r\n", m_SendFrom, m_SendFrom );
            smtpSocket.Write( buffer, strlen(buffer) );
            
            if ( TRUE == m_CancelSend ) break;

            _snprintf( buffer, sizeof(buffer) - 1, "To: \"%s\" <%s>\r\n", m_SendTo, m_SendTo );
            smtpSocket.Write( buffer, strlen(buffer) );

            if ( TRUE == m_CancelSend ) break;

            _snprintf( buffer, sizeof(buffer) - 1, "Subject: %s\r\n", m_pSubject );
            smtpSocket.Write( buffer, strlen(buffer) );

            if ( TRUE == m_CancelSend ) break;
            
            pString = "MIME-Version: 1.0\r\n"
                      "Content-Type: multipart/mixed;\r\n"
                      "\tboundary=\"_0c1758cc-675d-478f-8e59-c28123f784b2_\"\r\n\r\n";
            smtpSocket.Write( pString, strlen(pString) );

            if ( TRUE == m_CancelSend ) break;

            //begin boundary for message content
            pString = "--_0c1758cc-675d-478f-8e59-c28123f784b2_\r\n"
                      "Content-Type: text/plain; charset=\"iso-8859-1\"\r\n"
                      "Content-Transfer-Encoding: quoted-printable\r\n\r\n";
            smtpSocket.Write( pString, strlen(pString) );

            smtpSocket.Write( m_pMessage, strlen(m_pMessage) );

            pString = "\r\n\r\n";
            smtpSocket.Write( pString, strlen(pString) );
            
            //write everything to this point
            while ( 0 != smtpSocket.GetWriteSize( ) )
            {
               smtpSocket.Write( );

               if ( TRUE == m_CancelSend ) break;
            }

            if ( TRUE == m_CancelSend ) break;

            const char *pPic[] =
            {
               "pic1.jpg",
               "pic2.jpg"
            };

            char *pEncode = NULL;
            char text[ 1024 ] = { 0 };

            //encode and write the attacments one at a time
            //sending after each one so our write buffer doesn't grow huge
            for ( i = 0; i < m_NumAttachments; i++ )
            {
               //boundary for each attachment
               pString = "--_0c1758cc-675d-478f-8e59-c28123f784b2_\r\n"
                         "Content-Type: %s; name=%s\r\n"
                         "Content-Transfer-Encoding: base64\r\n"
                         "Content-Disposition: attachment; filename=\"%s\"\r\n\r\n";

               _snprintf( text, sizeof(text) - 1, pString, AttachmentTypeToString(m_Attachments[ i ].type), m_Attachments[ i ].name, m_Attachments[ i ].name ); 
               
               smtpSocket.Write( text, strlen(text) );

               size_t size = b64size( m_Attachments[ i ].size );

               pEncode = (char *) realloc( pEncode, size );

               b64encode( pEncode, size, (const char *) m_Attachments[ i ].pData, m_Attachments[ i ].size );
               smtpSocket.Write( pEncode, strlen(pEncode) );

               pString = "\r\n\r\n";
               smtpSocket.Write( pString, strlen(pString) );

               //block until it's sent so our buffer doesn't
               //grow too huge
               while ( 0 != smtpSocket.GetWriteSize( ) )
               {
                  smtpSocket.Write( );

                  if ( TRUE == m_CancelSend ) break;
               }

               if ( TRUE == m_CancelSend ) break;
            }

            free( pEncode );

            if ( TRUE == m_CancelSend ) break;

            //end boundary for our email
            pString = "--_0c1758cc-675d-478f-8e59-c28123f784b2_--";
            smtpSocket.Write( pString, strlen(pString) );

            smtpSocket.Write( "\r\n.\r\n", strlen("\r\n.\r\n") );

            if ( TRUE == m_CancelSend ) break;

            //write remaining data
            while ( 0 != smtpSocket.GetWriteSize( ) )
            {
               smtpSocket.Write( );

               if ( TRUE == m_CancelSend ) break;
            }

             if ( TRUE == m_CancelSend ) break;
            
            //receive "ok"
            while ( FALSE == smtpSocket.Read( ) )
            {
               if ( TRUE == m_CancelSend ) break;
            }

            if ( TRUE == m_CancelSend ) break;

            smtpSocket.WriteImmediate( "QUIT", strlen("QUIT") );

            g_Log.Write( Log::TypeInfo, "Email: Succesfully Sent" );

         } while ( 0 );

         smtpSocket.Destroy( );
         
         if ( pCtx )
         {
            SSL_CTX_free( pCtx );
         }

         ERR_remove_state( 0 );
      
         delete [] m_pMessage;
         delete [] m_pSubject;

         m_pMessage = NULL;
         m_pSubject = NULL;
         
         for ( i = 0; i < m_NumAttachments; i++ )
         {
            free( m_Attachments[ i ].pData );
         }

         m_NumAttachments = 0;

      } //thread lock

      SuspendThread( m_hThread );
   }
}

DWORD WINAPI SmtpSender::ThreadProc(
   void *pParam
)
{
   SmtpSender *pSender = (SmtpSender *) pParam;
   pSender->RunThread( );
   
   return 0;
}
