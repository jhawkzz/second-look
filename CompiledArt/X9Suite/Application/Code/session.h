
#ifndef SESSION_H_
#define SESSION_H_

#include "Socket.h"
#include "SessionId.h"
#include "HashTable.h"
#include "List.h"

class Session
{
protected:
   struct PostData
   {
      char key[ 32 ];
      char value[ 32 ];
   };

protected:
   static const uint32 MaxPostDatas = 256;
   static const uint32 MaxUserDatas = 256;

protected:
   HashTable<const char *, const void *> m_UserDataHash;
   List<Socket *> m_SocketList;

   PostData    m_PostDatas[ MaxPostDatas ];
   SessionId   m_SessionId;
   Socket     *m_pActiveSocket;
   float       m_Timeout;
   float       m_MaxSessionTime;
   uint32      m_NumPostDatas;
   uint32      m_SocketEnumIndex;
   BOOL        m_IsLoggedIn;
   BOOL        m_IdVerfied;
   char        m_HttpRequest[ MAX_PATH ];
   char        m_Type[ 32 ];

public:
   Session( void ) :
      m_UserDataHash( 8, 8 )
   {
      m_IsLoggedIn = FALSE;
   }

   void Create(
      float timeout
   );

   void Destroy( void );

   void AddSocket(
      Socket *pSocket
   );

   void ResetSocketEnum( void );

   Socket *GetNextSocket( void );

   BOOL ReadHTTP( void );
   
   const char *RequestedPage( void );

   void BeginSendHTTP(
      const char *pType = "text/html"
   );
   
   void SendHTTP( 
      const char *pString
   );
   
   void SendHTTP( 
      const void *pData, 
      size_t size 
   );
   
   void EndSendHTTP( void );

   const char *GetPostData( 
      const char *pKey,
      uint32 index = 0
   );

   BOOL ValidateAddress(
      in_addr *__w64
   ) const;

   BOOL IsOpen( void );

   BOOL HasExpired( void ) const;

   void LogIn( void );

   void SetUserData(
      const char *pKey,
      const void *pData
   );

   const void *GetUserData(
      const char *pKey
   ) const;

   void ClearUserData( void )
   {
      m_UserDataHash.Clear( );
   }

   void LogOut( void ) { m_IsLoggedIn = FALSE; ClearUserData( ); }

   void SetActiveSocket( Socket *pSocket ) { m_pActiveSocket = pSocket; }

   BOOL IsSecure( void ) const 
   { 
      return TRUE  == m_IsLoggedIn  && 
             FALSE == HasExpired( ) &&
             TRUE  == m_IdVerfied; }

protected:
   void ParseHTTPRequest( void );
   void ParseHTTPGet( void );
   void ParseHTTPPost( void );
   
   void VerifySession(
      const SessionId *pSessionId
   );
   
   void FillPostData(
      const char *pRequest,
      size_t length
   );

   void CleanupInvalidSockets( void );
   
   void DecodeUrl(
      char *pDecoded,
      const char *pSource,
      size_t decodedSize
   );
};

#endif
