
#ifndef LOG_H_
#define LOG_H_

#include "ThreadLock.h"
#include "commandBuffer.h"

class Log : public ICommand
{
public:
   enum Type
   {
      TypeInfo,
      TypeWarning,
      TypeError,
   };

   struct Message
   {
      Type type;
      char message[ 1024 ];
   };

protected:
   static const uint32 MaxMessages = 128;

protected:
   Lock    m_Lock;
   Message m_Messages[ MaxMessages ];
   uint32  m_CurrentMessage;
   char    m_File[ MAX_PATH ];

public:
   void Initialize(
      const char *pPath
   );

   void Uninitialize( void );

   void ExecuteCommand( 
      Command *pCommand 
   );

   void Write(
      Type type,
      const char *pMessage,
      ...
   );

   const Message *GetMesssage( 
      uint32 index
   ) const;

   const char *GetCommandName( void ) { return "log"; }

   void Clear( void ) { m_CurrentMessage = 0; }

   void DumpFile(
      const char *pPath   
   );

protected:
   void WriteMessage(
      Type type,
      const char *pMessage
   );
};

#endif //LOG_H_
