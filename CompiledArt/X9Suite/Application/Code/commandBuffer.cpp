
#include "precompiled.h"
#include "main.h"

const char *ICommand::Command::GetString( void )
{
   if ( NULL == pCurrent )
   {
      pCurrent = buffer;
   }
   else
   {
      pCurrent += strlen( pCurrent ) + 1;

      if ( 0 == *pCurrent )
      {
         pCurrent = NULL;
      }
   }
   
   return pCurrent;
}

BOOL CommandBuffer::Register(
   ICommand *pICommand
)
{
   ThreadLock lock( m_Lock );

   uint32 i;

   if ( MaxICommands == m_NumICommands )
   {
      g_Log.Write( Log::TypeError, "Maximum number of commands have been registered" );
      return FALSE;
   }

   for ( i = 0; i < m_NumICommands; i++ )
   {
      if ( 0 == strcmpi(m_pICommands[ i ]->GetCommandName( ), pICommand->GetCommandName( )) )
      {
         break;
      }
   }

   if ( i < m_NumICommands ) 
   {
      g_Log.Write( Log::TypeError, "Duplicate command %s attempted to register", pICommand->GetCommandName( ) );
      return FALSE;
   }

   m_pICommands[ m_NumICommands++ ] = pICommand;
   
   g_Log.Write( Log::TypeInfo, "Command %s registered", pICommand->GetCommandName( ) );
   
   return TRUE;
}

void CommandBuffer::Unregister(
   ICommand *pICommand
)
{
   ThreadLock lock( m_Lock );

   uint32 i;

   for ( i = 0; i < m_NumICommands; i++ )
   {
      if ( pICommand == m_pICommands[ i ] )
      {
         m_pICommands[ i ] = m_pICommands[ --m_NumICommands ];
         break;
      }
   }
}

void CommandBuffer::Write(
   const char *pMessage,
   ...
)
{
   ThreadLock lock( m_Lock );

   if ( MaxQueuedCommands == m_NumQueuedCommands )
   {
      _ASSERTE( ! "Increase the command buffer queue size" );
      return;
   }
   
   ICommand::Command *pCommand = &m_QueuedCommands[ m_NumQueuedCommands ];

   va_list marker;
   
   va_start( marker, pMessage );

   _vsnprintf( pCommand->buffer, sizeof(pCommand->buffer) - 1, pMessage, marker );

   //end w/ double terminator
   pCommand->buffer[ sizeof(pCommand->buffer) - 1 ] = NULL;
   pCommand->buffer[ sizeof(pCommand->buffer) - 2 ] = NULL;

   size_t length = strlen( pCommand->buffer );

   //change all spaces to null terminators unless
   //it's in quotes
   char *pDest   = pCommand->buffer;
   char *pSource = pCommand->buffer;

   BOOL inQuotes = FALSE;

   size_t i;

   for ( i = 0; i < length; i++ )
   {
      if ( ' ' == *pSource && FALSE == inQuotes )
      {
         *pDest = NULL; 
         ++pDest;
      }
      else if ( '"' == *pSource )
      {
         inQuotes = ! inQuotes;
      }
      else
      {
         *pDest = *pSource;         
         ++pDest;
      }

      ++pSource;
   }

   *pDest = NULL;

   pCommand->pCurrent = NULL;

   va_end( marker );

   ++m_NumQueuedCommands;
}

void CommandBuffer::FlushCommands( void )
{
   ThreadLock lock( m_Lock );

   uint32 i, c;

   for ( i = 0; i < m_NumQueuedCommands; i++ )
   {
      const char *pCommand = m_QueuedCommands[ i ].GetString( );
      
      for ( c = 0; c < m_NumICommands; c++ )
      {
         if ( 0 == strcmpi(pCommand, m_pICommands[ c ]->GetCommandName()) )
         {
            m_pICommands[ c ]->ExecuteCommand( &m_QueuedCommands[ i ] );
            break;
         }
      }

      if ( c == m_NumICommands )
      {
         g_Log.Write( Log::TypeError, "unrecognized command \"%s", pCommand );
      }
   }

   m_NumQueuedCommands = 0;
}
