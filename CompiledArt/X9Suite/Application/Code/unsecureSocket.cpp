
#include "precompiled.h"
#include "main.h"
#include "unsecureSocket.h"

BOOL UnsecureSocket::Read( void )
{
   _ASSERTE( TRUE == IsValid( ) && "Socket not valid" );
   
   char buffer[ 1024 ];

   int result = recv( m_Socket, buffer, (int) sizeof(buffer), 0 );
   
   if ( 0 == result )
   {
      Destroy( );
   }
   else if ( SOCKET_ERROR == result )
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

void UnsecureSocket::Write( void )
{
   _ASSERTE( TRUE == IsValid( ) && "Socket not valid" );

   size_t size = m_OutgoingStream.GetSize( ) > 1024 * 10 ? 1024 * 10 : m_OutgoingStream.GetSize( ); 

   int result = send( m_Socket, (const char *) m_OutgoingStream.GetBuffer( ), (int) size, 0 );
   
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