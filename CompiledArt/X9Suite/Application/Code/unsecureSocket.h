
#ifndef UNSECURESOCKET_H_
#define UNSECURESOCKET_H_

#include "Socket.h"

class UnsecureSocket : public Socket
{
public:
   using Socket::Write;

public:
   BOOL Read ( void );
   void Write( void );
};

#endif
