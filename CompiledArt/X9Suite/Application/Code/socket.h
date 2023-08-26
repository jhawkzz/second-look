
#ifndef SOCKET_H_
#define SOCKET_H_

#include "memoryStream.h"

class Socket
{
protected:
   MemoryStream m_IncomingStream;
   MemoryStream m_OutgoingStream;
   sockaddr_in  m_Address;
   SOCKET       m_Socket;

public:
   Socket( void ) 
   {
      m_Socket = INVALID_SOCKET;
      memset( &m_Address, 0, sizeof(m_Address) );
   }

public:
   virtual void Create(
      SOCKET socket,
      sockaddr_in *pAddress
   );

   virtual void Destroy( void );

   virtual BOOL Read ( void ) = 0;

   virtual void Write( void ) = 0;

   virtual BOOL SetServer( void ) { return TRUE; }

public:
   void Dump(
      const char *pFilename
   );

   void Insert(
      uint32 pos,
      const void *pData,
      size_t size
   );

   void Write(
      const void *pData,
      size_t size
   );

   void WriteImmediate(
      const void *pData,
      size_t size
   );

   BOOL IsAddressEqualTo(
      in_addr *__w64 pAddress
   ) const;
   
   void *GetReadBuffer( void )
   {
      return m_IncomingStream.GetBuffer( );
   }

   size_t GetReadSize( void )
   {
      return m_IncomingStream.GetSize( );
   }

   size_t GetWriteSize( void )
   {
      return m_OutgoingStream.GetSize( );
   }

   void ReadClear( void )
   {
      m_IncomingStream.Clear( );
   }

   void WriteClear( void )
   {
      m_OutgoingStream.Clear( );
   }

   void TerminateReadBuffer( void )
   {
      char *pBuffer = (char *) m_IncomingStream.GetBuffer( );

      if ( *(pBuffer + m_IncomingStream.GetSize( ) - 1) )
      {
         char zero = 0;
         m_IncomingStream.Write( &zero, 1 );
      }
   }

   BOOL IsValid( void ) const { return INVALID_SOCKET != m_Socket; }
};

#endif
