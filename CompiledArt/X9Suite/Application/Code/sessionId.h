
#ifndef SESSIONID_H_
#define SESSIONID_H_

class SessionId
{
protected:
   static SessionId InvalidSession;

protected:
   UUID m_Guid;
   unsigned char *m_pString;

public:
   SessionId( void )
   {
      memset( &m_Guid, 0, sizeof(m_Guid) );
      m_pString = NULL;
   }

   ~SessionId( void )
   {
      if ( m_pString )
      {
         RpcStringFree( &m_pString );
      }
   }

   SessionId( 
      const SessionId &sessionId
   )
   {
      memcpy( &m_Guid, &sessionId.m_Guid, sizeof(m_Guid) );
      UuidToString( &m_Guid, &m_pString );
   }

   void Create( void )
   {
      UuidCreate( &m_Guid );
      UuidToString( &m_Guid, &m_pString );
   };

   const char *ToString( void ) 
   { 
      return (char *) m_pString;
   }

   bool operator == (const SessionId &sessionId) const
   {  
      RPC_STATUS status;
      
      SessionId nonConstId(sessionId);
      UUID nonConstMyId = m_Guid;

      if ( UuidEqual(&InvalidSession.m_Guid, &nonConstId.m_Guid, &status) ) return false;
      return (TRUE == UuidEqual(&nonConstMyId, &nonConstId.m_Guid, &status));
   }

public:
   static void Parse( 
      char *pOutString,
      size_t size,
      SessionId *pSessionId,
      const char *pString
   )
   {
      //first copy the entire string incase
      //there is nothing to parse
      strncpy( pOutString, pString, size );
      pOutString[ size - 1 ] = NULL;

      *pSessionId = SessionId::InvalidSession;

      if ( NULL == pString ) return;

      //ensure there is a ? in the string
      const char *pStart = strchr( pString, '?' );
      if ( NULL == pStart ) return;     

      //copy the string so we can start working with it
      strncpy( pOutString, pString, size );
      pOutString[ size - 1 ] = NULL;

      //terminate so we can return just what is before the string
      char *pTerminate = strchr( pOutString, '?' );
      if ( NULL == pTerminate ) return;     
      *pTerminate = NULL;

      ++pStart;
      
      UuidFromString( const_cast<unsigned char *>((const unsigned char *)pStart), &pSessionId->m_Guid );
      /*
      memset( pSessionId->value, 0, sizeof(pSessionId->value) );

      uint32 i;
      size_t length = strlen(pStart);
      
      length = min( length, sizeof(pSessionId->value) );

      for ( i = 0; i < length; i++ )
      {
         pSessionId->value[ i ] = pStart[ i ];
      }*/
   }
};

#endif
