
#ifndef COMMANDBUFFER_H_
#define COMMANDBUFFER_H_

#include "ThreadLock.h"

class ICommand
{
public:
   struct Command
   {
      const char *pCurrent;
      char buffer[ 1024 ];
      
      const char *GetString( void );
   };

public:
   virtual const char *GetCommandName( void ) = 0;

   virtual void ExecuteCommand( 
      Command *pCommand 
   ) = 0;
};

class CommandBuffer
{
protected:

protected:
   static const uint32 MaxICommands = 1024;
   static const uint32 MaxQueuedCommands = 64;

protected:
   ICommand::Command m_QueuedCommands[ MaxQueuedCommands ];

   ICommand *m_pICommands[ MaxICommands ];
   uint32    m_NumQueuedCommands;
   uint32    m_NumICommands;
   Lock      m_Lock;

public:
   CommandBuffer( void )
   {
      m_NumICommands = 0;
      m_NumQueuedCommands = 0;
   }

   BOOL Register( 
      ICommand *pICommand 
   );

   void Unregister(
      ICommand *pICommand
   );

   void Write(
      const char *pMessage,
      ...
   );
   
   void FlushCommands( void );
};

#endif //COMMANDBUFFER_H_
