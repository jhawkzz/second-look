
#include "precompiled.h"
#include "main.h"

void Session::Create(
   float timeout   
)
{
   m_SessionId.Create( );
      
   m_MaxSessionTime = timeout;

   m_NumPostDatas = 0;

   m_IsLoggedIn = FALSE;

   m_Timeout = Timer::m_total + m_MaxSessionTime;
}

void Session::Destroy( void )
{
   CleanupInvalidSockets( );

   //and then destroy and remaining open sockets
   uint32 i, size = (uint32) m_SocketList.GetSize( );

   for ( i = 0; i < size; i++ )
   {
      m_SocketList.Get( i )->Destroy( );
      delete m_SocketList.Get( i );
   }

   m_SocketList.Clear( );
}

void Session::AddSocket(
   Socket *pSocket
)
{
   CleanupInvalidSockets( );
   
   m_SocketList.Add( pSocket );
}

void Session::ResetSocketEnum( void )
{
   m_SocketEnumIndex = -1;
}

Socket *Session::GetNextSocket( void )
{
   uint32 i, size = (uint32)m_SocketList.GetSize( );

   for ( i = m_SocketEnumIndex + 1; i < size; i++ )
   {
      if ( TRUE == m_SocketList.Get( i )->IsValid( ) )
      {
         m_SocketEnumIndex = i;
         return m_SocketList.Get( i );
      }
   }

   return NULL;
}

BOOL Session::ReadHTTP( void )
{
   BOOL dataRead = FALSE;

   //if we still need to become the server this time around, that is all we 
   //will do and we'll handle the next on the next tick
   if ( FALSE == m_pActiveSocket->SetServer( ) )
   {
      return dataRead;
   }

   BOOL stillReading = m_pActiveSocket->Read( );

   //prevent too much data from being read, possibly
   //from a program trying to exploit us
   if ( m_pActiveSocket->GetReadSize( ) > 1024 * 1024 )
   {
      stillReading = FALSE;
   }

   //if we're done reading and we did read data
   if ( FALSE == stillReading && m_pActiveSocket->GetReadSize( ) )
   {
      m_pActiveSocket->TerminateReadBuffer( );
      m_pActiveSocket->WriteClear( );

      ParseHTTPRequest( );
      
      m_pActiveSocket->ReadClear( );

      dataRead = TRUE;
   }

   if ( m_pActiveSocket->GetWriteSize( ) )
   {
      m_pActiveSocket->Write( );
   }

   return dataRead;
}

const char *Session::RequestedPage( void )
{
   return m_HttpRequest;
}

void Session::BeginSendHTTP(
   const char *pType // = "text/html"
)
{
   strncpy( m_Type, pType, sizeof(m_Type) - 1 );
   m_Type[ sizeof(m_Type) - 1 ] = NULL;
}

void Session::SendHTTP(
   const char *pString
)
{
   size_t length = strlen(pString);   

   SendHTTP( pString, length );
}

void Session::SendHTTP( 
   const void *pVoidData, 
   size_t size
)
{
   //insert session id where appropriate
   size_t i = 0;
   size_t sessionSize = sizeof("<sessionId>") - 1;

   const char *pData = (const char *) pVoidData;

   if ( size > sessionSize )
   {
      for ( i = 0; i < size; i++ )
      {
         if ( pData[ i ] == '<' )
         {
            //if the buffer has enough for <sessionId>
            //and it says <sessionId>
            if ( size - i > sessionSize 
                 && 0 == memcmp(pData + i, "<sessionId>", sessionSize) )
            {
               m_pActiveSocket->Write( m_SessionId.ToString( ), strlen(m_SessionId.ToString( )) );
               i += sessionSize - 1;
               continue;
            }
         }

         m_pActiveSocket->Write( &pData[ i ], 1 );
      }
   }

   //write remaining data
   m_pActiveSocket->Write( pData + i, size - i );
}

void Session::EndSendHTTP( void )
{
   char string[ 256 ] = { 0 };
      
   //send the header before the rest of the information
   //so the web browser knows the data type
   _snprintf( string, sizeof(string) - 1, "HTTP/1.1 200 OK\r\n"
                                          "Content-Length: %d\r\n"
                                          "Content-Type: %s\r\n\r\n", m_pActiveSocket->GetWriteSize( ), m_Type );

   m_pActiveSocket->Insert( 0, string, strlen(string) );
}

const char *Session::GetPostData( 
   const char *pKey,
   uint32 index// = 0
)
{
   uint32 i, current = 0;

   for ( i = 0; i < m_NumPostDatas; i++ )
   {
      if ( 0 == strcmp(pKey, m_PostDatas[ i ].key) )
      {
         if ( current == index )
         {
            return m_PostDatas[ i ].value;
         }

         ++current;
      }
   }
   
   return NULL;
}


BOOL Session::ValidateAddress(
   in_addr *__w64 pAddress
) const
{
   uint32 i, size = (uint32)m_SocketList.GetSize( );

   for ( i = 0; i < size; i++ )
   {
      if ( TRUE == m_SocketList.Get( i )->IsAddressEqualTo(pAddress) )
      {
         break;
      }
   }

   return ( i < size );
}

BOOL Session::IsOpen( void )
{
   uint32 i, size = (uint32)m_SocketList.GetSize( );

   if ( FALSE == HasExpired( ) ) return TRUE;

   for ( i = 0; i < size; i++ )
   {
      if ( TRUE == m_SocketList.Get( i )->IsValid( ) )
      {
         return TRUE;
      }
   }

   return FALSE;
}

BOOL Session::HasExpired( void ) const
{
   if ( m_Timeout - Timer::m_total > 0.0f )
   {
      return FALSE;
   }

   return TRUE;
}

void Session::LogIn( void )
{
   m_IsLoggedIn = TRUE;
   
   m_Timeout = Timer::m_total + m_MaxSessionTime;
}

void Session::SetUserData(
   const char *pKey,
   const void *pData
)
{
   m_UserDataHash.Remove( pKey );
   m_UserDataHash.Add( pKey, pData );
}

const void *Session::GetUserData(
   const char *pKey
) const
{
   const void *pData = NULL;
   
   m_UserDataHash.Get( pKey, &pData );

   return pData;
}

void Session::ParseHTTPRequest( )
{
   char *pRequest;

   //search the buffer for the word GET in all uppercase
   pRequest = strstr( (char *) m_pActiveSocket->GetReadBuffer( ), "GET" );

   if ( pRequest ) 
   {
      ParseHTTPGet( );
   }
   else
   {
      pRequest = strstr( (char *) m_pActiveSocket->GetReadBuffer( ), "POST" );

      if ( pRequest )
      {
         ParseHTTPPost( );
      }
   }
}

void Session::ParseHTTPGet( void )
{
   char linkRequested[ MAX_PATH ];

   char *pRequest;

   //search the buffer for the word GET in all uppercase
   pRequest = strstr( (char *) m_pActiveSocket->GetReadBuffer( ), "GET" );

   //if we found a GET, look for the file requested
   if ( pRequest )
   {
      //the file requested will come after the first '/'
      pRequest = strchr( pRequest, '/' );

      //if there is not a space after the '/'
      //then there is a filename
      if ( ' ' != pRequest[ 1 ] ) 
      {
         ++pRequest;

         //search for the characters that come after the
         //end of the filename (" HTTP").  we will chop
         //them off and set it to null
         char *pEnd = strstr( pRequest, " HTTP" );
         if ( pEnd ) *pEnd = NULL;

         //now pRequest points to the beginning of our filename
         //and null is directly after the filename...copy it
         //into our linkRequested array so we know which file to load
         strncpy( linkRequested, pRequest, MAX_PATH );

         //set the pointer to point to linkRequested - because it could
         //get screwed up if it continues to point to "buffer"
         pRequest = linkRequested;
      }
      else 
      {
         pRequest = "";
      }
   }
   else
   {
      pRequest = "";
   }

   SessionId sessionId;
   SessionId::Parse( m_HttpRequest, sizeof(m_HttpRequest), &sessionId, pRequest );

   VerifySession( &sessionId );
}

void Session::ParseHTTPPost( void )
{
   char linkRequested[ MAX_PATH ];

   char *pRequest;
   char *pPostData;

   //search the buffer for the word GET in all uppercase
   pRequest = strstr( (char *) m_pActiveSocket->GetReadBuffer( ), "POST" );

   //if we found a POST, look for the file requested
   if ( pRequest )
   {
      //the file requested will come after the first '/'
      pRequest = strchr( pRequest, '/' );

      //if there is not a space after the '/'
      //then there is a filename
      if ( ' ' != pRequest[ 1 ] ) 
      {
         ++pRequest;

         pPostData = strstr( pRequest, "\r\n\r\n" );
         pPostData += 4;

         //search for the characters that come after the
         //end of the filename (" HTTP").  we will chop
         //them off and set it to null
         char *pEnd = strstr( pRequest, " HTTP" );
         if ( pEnd ) *pEnd = NULL;

         //now pRequest points to the beginning of our filename
         //and null is directly after the filename...copy it
         //into our linkRequested array so we know which file to load
         strncpy( linkRequested, pRequest, MAX_PATH );

         FillPostData( pRequest,  (size_t) ((char *) m_pActiveSocket->GetReadSize( )) - (pRequest - (char *) m_pActiveSocket->GetReadBuffer( )) );

         //set the pointer to point to linkRequested - because it could
         //get screwed up if it continues to point to "buffer"
         pRequest = linkRequested;
      }
      else 
      {
         pRequest = "";
      }
   }
   else
   {
      pRequest = "";
   }
   
   SessionId sessionId;
   SessionId::Parse( m_HttpRequest, sizeof(m_HttpRequest), &sessionId, pRequest );

   VerifySession( &sessionId );
}

void Session::VerifySession(
   const SessionId *pSessionId
)
{
   m_IdVerfied = FALSE;
   
   if ( FALSE == HasExpired( ) )
   {
      if ( *pSessionId == m_SessionId )
      {
         m_Timeout   = Timer::m_total + m_MaxSessionTime;
         m_IdVerfied = TRUE;
      }
   }
}

void Session::FillPostData(
   const char *pRequest,
   size_t requestSize
)
{
   const char *pPostData, *pEnd;
   size_t length;

   m_NumPostDatas = 0;

   if ( NULL == pRequest ) return;

   pPostData = pRequest + strlen(pRequest) + 1;
   pPostData = strstr( pPostData, "\r\n\r\n" );
   pPostData += 4;

   while ( (size_t) (pPostData - pRequest) < requestSize )
   {
      pEnd = strchr( pPostData, '=' );
      if ( NULL == pEnd ) break;

      length = pEnd - pPostData;
      length = min( length, sizeof(m_PostDatas[ m_NumPostDatas ].key) - 1 );
      DecodeUrl( m_PostDatas[ m_NumPostDatas ].key, pPostData, length );
      m_PostDatas[ m_NumPostDatas ].key[ length ] = NULL;

      pPostData = pEnd + 1;
      pEnd = strchr( pPostData, '&' );
      if ( NULL == pEnd ) 
      {
         pEnd = pRequest + requestSize;
      }

      length = pEnd - pPostData;
      length = min( length, sizeof(m_PostDatas[ m_NumPostDatas ].value) - 1 );
      DecodeUrl( m_PostDatas[ m_NumPostDatas ].value, pPostData, length );
      m_PostDatas[ m_NumPostDatas ].value[ length ] = NULL;

      pPostData = pEnd + 1;
      ++m_NumPostDatas;

      if ( MaxPostDatas == m_NumPostDatas ) break;
   }
}

void Session::CleanupInvalidSockets( void )
{
   uint32 i;

   for ( i = 0; i < m_SocketList.GetSize( ); i++ )
   {
      Socket *pSocket = m_SocketList.Get( i );
      
      if ( FALSE == pSocket->IsValid( ) )
      {
         m_SocketList.Remove( i );
         --i;
         
         pSocket->Destroy( );
         delete pSocket;
      }
   }
}

void Session::DecodeUrl(
   char *pDecoded,
   const char *pSource,
   size_t decodedSize
)
{
   const char *pBase = pSource;
   char *pCurrent = pDecoded;

   //html codes referenced from: http://www.eskimo.com/~bloo/indexdot/html/topics/urlencoding.htm
   while ( *pSource )
   {
      if ( pSource - pBase == decodedSize ) break;
      
      if      ( 0 == strnicmp(pSource, "%24", 3) ) { *pCurrent = 36;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%26", 3) ) { *pCurrent = 38;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%2b", 3) ) { *pCurrent = 43;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%2c", 3) ) { *pCurrent = 44;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%2f", 3) ) { *pCurrent = 47;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%3a", 3) ) { *pCurrent = 58;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%3b", 3) ) { *pCurrent = 59;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%3d", 3) ) { *pCurrent = 61;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%3f", 3) ) { *pCurrent = 63;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%40", 3) ) { *pCurrent = 64;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%20", 3) ) { *pCurrent = 32;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%22", 3) ) { *pCurrent = 34;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%3c", 3) ) { *pCurrent = 60;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%3e", 3) ) { *pCurrent = 62;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%23", 3) ) { *pCurrent = 35;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%25", 3) ) { *pCurrent = 37;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%7b", 3) ) { *pCurrent = 123; pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%7d", 3) ) { *pCurrent = 125; pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%7c", 3) ) { *pCurrent = 124; pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%5c", 3) ) { *pCurrent = 92;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%5e", 3) ) { *pCurrent = 94;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%7e", 3) ) { *pCurrent = 126; pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%5b", 3) ) { *pCurrent = 91;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%5d", 3) ) { *pCurrent = 93;  pSource += 3; }
      else if ( 0 == strnicmp(pSource, "%60", 3) ) { *pCurrent = 96;  pSource += 3; }
      else
      {
         *pCurrent = *pSource;
         ++pSource;
      }

      ++pCurrent;
   }

   *pCurrent = NULL;
   pCurrent[ decodedSize - 1 ] = NULL;
}


