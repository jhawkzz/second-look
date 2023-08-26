
// copyright © 2005 PBJ Games
#include "precompiled.h"

#include "registry_class.h"
#include <shlwapi.h>

Registry_Class::Registry_Class( HKEY root_key /*=HKEY_CURRENT_USER*/ )
{
   m_key_handle = NULL;
   m_root_key   = root_key;
}

Registry_Class::~Registry_Class( )
{
   Close_Key( );
}

void Registry_Class::Set_Root_Key( HKEY rootKey )
{
   m_root_key = rootKey;
}

BOOL Registry_Class::Open_Key( const char * key, REGSAM samDesired, BOOL createOnFail /*=TRUE*/ )
{
   BOOL success = TRUE;

   Close_Key( );

   // if this is true, just try to make the key now. If it already exists then create will fail.
   if ( createOnFail )
   {
      Create_Key( key );
   }

   if ( RegOpenKeyEx( m_root_key, key, 0, samDesired, &m_key_handle ) != ERROR_SUCCESS )
   {
      success = FALSE;
   }

   return success;
}

BOOL Registry_Class::Does_Key_Exist( char * key )
{
   BOOL exists  = FALSE;
   HKEY new_key = NULL;

	LONG result = RegOpenKeyEx( m_root_key, key, 0, KEY_READ, &new_key );

   if ( ERROR_SUCCESS == result )
   {
      exists = TRUE;
   }

   if ( new_key )
   {
      RegCloseKey( new_key );
   }

   return exists;
}

void Registry_Class::Close_Key( void )
{
   if ( m_key_handle )
   {
      RegCloseKey( m_key_handle );

      m_key_handle = NULL;
   }
}

BOOL Registry_Class::Get_Value_Data_String( char * value, char * data, DWORD buffer_length, const char * default_data )
{
   BOOL success = FALSE;

   do
   {
      if ( !m_key_handle ) break;

      DWORD type = REG_SZ;

      if ( RegQueryValueEx( m_key_handle, value, 0, &type, (BYTE *)data, &buffer_length ) != ERROR_SUCCESS ) break;

      success = TRUE;
   }
   while( 0 );

   if ( !success )
   {
      if ( default_data )
      {
         strcpy( data, default_data );
      }
   }

   return success;
}

DWORD Registry_Class::Get_Value_Data_Int( char * value, int default_data, BOOL * success )
{
   BOOL result = FALSE;

   DWORD data = default_data;

   do
   {
      if ( !m_key_handle ) break;

      DWORD type = REG_DWORD;
      DWORD buffer_size = sizeof( data );

      if ( RegQueryValueEx( m_key_handle, value, 0, &type, (BYTE *)&data, &buffer_size ) != ERROR_SUCCESS ) break;

      result = TRUE;
   }
   while( 0 );

   if ( !result && success )
   {
      *success = result;
   }

   return data;
}

BOOL Registry_Class::Get_Value_Data_Binary( char * value, void * data, DWORD buffer_length, const BYTE * default_data )
{
   BOOL success = FALSE;

   do
   {
      if ( !m_key_handle ) break;

      DWORD type = REG_BINARY;

      if ( RegQueryValueEx( m_key_handle, value, 0, &type, (BYTE *) data, &buffer_length ) != ERROR_SUCCESS ) break;

      success = TRUE;
   }
   while( 0 );

   if ( !success )
   {
      if ( default_data )
      {
         memcpy( data, default_data, buffer_length );
      }
   }

   return success;
}

BOOL Registry_Class::Set_Value_Data_String( char * value, const char * data )
{
   BOOL success = FALSE;

   do
   {
      if ( !m_key_handle ) break;

      DWORD type = REG_SZ;
      if ( RegSetValueEx( m_key_handle, value, 0, REG_SZ, (BYTE *) data, (DWORD) strlen( data ) ) != ERROR_SUCCESS ) break;

      success = TRUE;
   }
   while( 0 );

   return success;
}

BOOL Registry_Class::Set_Value_Data_Int( char * value, DWORD data )
{
   BOOL success = FALSE;

   do
   {
      if ( !m_key_handle ) break;

      if ( RegSetValueEx( m_key_handle, value, 0, REG_DWORD, (BYTE *) &data, (DWORD) sizeof( data ) ) != ERROR_SUCCESS ) break;

      success = TRUE;
   }
   while( 0 );

   return success;
}

BOOL Registry_Class::Set_Value_Data_Binary( char * value, void * data, int size )
{
   BOOL success = FALSE;
 
   do
   {
      if ( !m_key_handle ) break;

      if ( RegSetValueEx( m_key_handle, value, 0, REG_BINARY, (BYTE *) data, size ) != ERROR_SUCCESS ) break;

      success = TRUE;
   }
   while( 0 );

   return success;
}

BOOL Registry_Class::Delete_Value( char * value )
{
   BOOL success = FALSE;

   do
   {
      if ( !m_key_handle ) break;

      if ( RegDeleteValue( m_key_handle, value ) != ERROR_SUCCESS ) break;
   }
   while( 0 );

   success = TRUE;

   return success;
}

BOOL Registry_Class::Delete_Key( char *pKeyToDelete, BOOL deleteSubKeys/* = FALSE*/ )
{
   if ( deleteSubKeys )
   {
      if ( SHDeleteKey( m_root_key, pKeyToDelete ) != ERROR_SUCCESS )
      {
         return FALSE;
      }
   }
   else
   {
      if ( RegDeleteKey( m_root_key, pKeyToDelete ) != ERROR_SUCCESS )
      {
         return FALSE;
      }
   }

   return TRUE;
}

BOOL Registry_Class::Create_Key( const char * key )
{
   BOOL success = TRUE;

   LSTATUS result = RegCreateKeyEx( m_root_key, key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &m_key_handle, NULL );
   if ( result != ERROR_SUCCESS )
   {
      success = FALSE;
   }

   return success;
}

int Registry_Class::Get_Key_Value_Count( void )
{
   int   valueCount   = 0;
   DWORD valueStrSize = MAX_PATH;
   char  valueStr[ MAX_PATH ];

   while ( ERROR_SUCCESS == RegEnumValue( m_key_handle, valueCount, valueStr, &valueStrSize, NULL, NULL, NULL, NULL ) )
   {
      valueCount++;
   }

   return valueCount;
}
