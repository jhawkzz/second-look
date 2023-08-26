
#include "precompiled.h"
#include "main.h"
#include "socket.h"

void Socket::Create(
   SOCKET socket,
   sockaddr_in *pAddress
)
{
   _ASSERTE( FALSE == IsValid( ) && "Socket still valid" );

   m_Socket = socket;
   memcpy( &m_Address, pAddress, sizeof(m_Address) );

   m_IncomingStream.Create( );
   m_OutgoingStream.Create( );
}

void Socket::Destroy( void )
{
   m_IncomingStream.Destroy( );
   m_OutgoingStream.Destroy( );

   if ( INVALID_SOCKET != m_Socket )
   {
      shutdown( m_Socket, SD_BOTH );
      closesocket( m_Socket );
   }

   m_Socket = INVALID_SOCKET;
}

void Socket::Dump(
   const char *pFilename
)
{
   _ASSERTE( TRUE == IsValid( ) && "Socket not valid" );
   
   FILE *pFile = fopen( pFilename, "wb" );

   fwrite( m_OutgoingStream.GetBuffer( ), m_OutgoingStream.GetSize( ), 1, pFile );

   fclose( pFile );
}

void Socket::Insert(
   uint32 pos,
   const void *pBuffer,
   size_t length
)
{
   m_OutgoingStream.Insert( pos, pBuffer, length );
}

void Socket::Write(
   const void *pBuffer,
   size_t length
)
{
   m_OutgoingStream.Write( pBuffer, length );
}

void Socket::WriteImmediate(
   const void *pBuffer,
   size_t length
)
{
   m_OutgoingStream.Write( pBuffer, length );

   while ( m_OutgoingStream.GetSize( ) )
   {
      Write( );
   }
}

BOOL Socket::IsAddressEqualTo(
   in_addr *__w64 pAddress
) const
{
   if ( 0 == memcmp(&m_Address.sin_addr, pAddress, sizeof(m_Address.sin_addr)) )
   {
      return TRUE;
   }
   
   return FALSE;
}
