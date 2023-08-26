
#include "precompiled.h"
#include "main.h"
#include "secureSocket.h"

BOOL SecureSocket::SetServer( void )
{
   if ( TRUE == m_NeedsHandshake )
   {
      SetSSLServer( );
      
      if ( TRUE == m_NeedsHandshake )
      {
         return FALSE;
      }
   }

   return TRUE;
}

void SecureSocket::Destroy( void )
{
   //shutdown instead of the baseclass
   //because we have to do it in the 
   //correct order with ssl
   shutdown( m_Socket, SD_BOTH );

   if ( m_pSSL )
   {
      SSL_shutdown( m_pSSL );
      SSL_free( m_pSSL );
   }

   closesocket( m_Socket );

   m_pSSL = NULL;
   m_Socket = INVALID_SOCKET;

   Socket::Destroy( );
}

BOOL SecureSocket::Read( void )
{
   _ASSERTE( TRUE == IsValid( ) && "Socket not valid" );
   
   char buffer[ 1024 ];

   int result = SSL_read( m_pSSL, buffer, (int) sizeof(buffer) );
   
   if ( SOCKET_ERROR == result )
   {
      result = WSAGetLastError( );
      
      if ( WSAEWOULDBLOCK != result )
      {
         Destroy( );
      }

      result = 0;
   }
   else
   {
      m_IncomingStream.Write( buffer, result );
   }

   return result > 0;
}

void SecureSocket::Write( void )
{
   _ASSERTE( TRUE == IsValid( ) && "Socket not valid" );

   size_t size = m_OutgoingStream.GetSize( ) > 1024 * 10 ? 1024 * 10 : m_OutgoingStream.GetSize( ); 

   int result = SSL_write( m_pSSL, m_OutgoingStream.GetBuffer( ), (int) size );
   
   if ( SOCKET_ERROR == result )
   {
      result = WSAGetLastError( );
      
      if ( WSAEWOULDBLOCK != result )
      {
         Destroy( );
      }
      
      result = 0;
   }

   m_OutgoingStream.Advance( result );
}

void SecureSocket::SetSSL(
   SSL *pSSL
)
{
   m_pSSL = pSSL;
   m_NeedsHandshake = TRUE;

   SSL_set_fd( m_pSSL, (int) m_Socket );
}

void SecureSocket::SetSSLClient( void )
{
   int err = SSL_connect( m_pSSL );
   
   if ( err >= 0 )
   {
      m_NeedsHandshake = FALSE;
   }
}

void SecureSocket::SetSSLServer( void )
{
   int attempts = 0;
   int err;

   do
   {
      err = SSL_accept( m_pSSL );
   
      if ( err >= 0 )
      {
         m_NeedsHandshake = FALSE;
         break;
      }

      Sleep( 1 );

      ++attempts;

   } while ( attempts < 10 );

   if ( TRUE == m_NeedsHandshake )
   {
      err = SSL_get_error( m_pSSL, err );

      if ( SSL_ERROR_SSL == err ||
           SSL_ERROR_SYSCALL == err )
      {
         //debugging code for error conditions
         const char *pError;
         char buffer[ 1024 ];

         err = ERR_get_error();
         ERR_error_string( err, buffer );
         
         pError = ERR_reason_error_string( err );
         
         Destroy( );
      }
   }
}
