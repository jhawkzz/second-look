
#include "precompiled.h"
#include "log.h"

void Log::Initialize(
   const char *pPath
)
{
   m_CurrentMessage = 0;


   SYSTEMTIME time;

   GetLocalTime( &time );   

   _snprintf( m_File, sizeof(m_File) - 1, "%s%02d.%02d.%02d.%02d.%02d.%02d.txt", 
      pPath, time.wMonth, time.wDay, time.wYear, time.wHour, time.wMinute, time.wSecond );
}

void Log::Uninitialize( void )
{
}

void Log::Write(
   Type type,
   const char *pMessage,
   ...
)
{
   ThreadLock lock( m_Lock );

   char buffer[ 1024 ];

   va_list marker;
   va_start( marker, pMessage );

   _vsnprintf( buffer, sizeof(buffer) - 1, pMessage, marker );
   buffer[ sizeof(buffer) - 1 ] = NULL;

   va_end( marker );
   
   WriteMessage( type, buffer );
}

void Log::ExecuteCommand(
   ICommand::Command *pCommand
)
{
   ThreadLock lock( m_Lock );

   const char *pType = pCommand->GetString( );

   if ( 0 == strcmpi(pType, "Error") )
   {
      WriteMessage( TypeError, pCommand->GetString( ) );
   }
   else if ( 0 == strcmpi(pType, "Warning") )
   {
      WriteMessage( TypeWarning, pCommand->GetString( ) );
   }
   else if ( 0 == strcmpi(pType, "Info") )
   {
      WriteMessage( TypeInfo, pCommand->GetString( ) );
   }
   else if ( 0 == strcmpi(pType, "clear") )
   {
      Clear( );
   }
}

const Log::Message *Log::GetMesssage( 
   uint32 index
) const
{
   if ( index >= m_CurrentMessage ) return NULL;

   return &m_Messages[ m_CurrentMessage - index - 1 ];
}

void Log::WriteMessage(
   Log::Type type,
   const char *pMessage
)
{
   SYSTEMTIME time;
   
   if ( m_CurrentMessage == MaxMessages )
   {
      --m_CurrentMessage;

      //move all messages back one and leave space at the end
      memmove( m_Messages, &m_Messages[ 1 ], m_CurrentMessage * sizeof(Message) );
   }

   GetLocalTime( &time );   
   
   m_Messages[ m_CurrentMessage ].type = type;

   _snprintf( m_Messages[ m_CurrentMessage ].message, sizeof(m_Messages[ m_CurrentMessage ].message) - 1, "%02d.%02d.%02d %02d:%02d:%02d %s", 
      time.wMonth, time.wDay, time.wYear, time.wHour, time.wMinute, time.wSecond, pMessage );


   FILE *pFile = fopen( m_File, "a" );

   if ( pFile )
   {
      fputs( m_Messages[ m_CurrentMessage ].message, pFile );
      fputs( "\r\n", pFile );
      
      fclose( pFile );
   }

   ++m_CurrentMessage;
}
