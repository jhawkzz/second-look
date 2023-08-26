
#ifndef SECURESOCKET_H_
#define SECURESOCKET_H_

#include "socket.h"

class SecureSocket : public Socket
{
public:
   using Socket::Write;

protected:
   SSL   *m_pSSL;
   BOOL   m_NeedsHandshake;

public:
   SecureSocket( void )
   {
      m_pSSL = NULL;
      m_NeedsHandshake = TRUE;
   }

public:
   virtual BOOL SetServer( void );

   virtual void Destroy( void );
   virtual BOOL Read ( void );
   virtual void Write( void );

public:
   void SetSSL(
      SSL *pSSL
   );
   
   void SetSSLClient( void );
   void SetSSLServer( void );

   BOOL GetNeedsHandshake( void ) const { return m_NeedsHandshake; }

};

#endif
