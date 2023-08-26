
// copyright © 2005 PBJ Games

#ifndef REGISTRY_CLASS_H_
#define REGISTRY_CLASS_H_

#define _CRT_SECURE_NO_DEPRECATE //Disable security warnings for stdio functions
#include <windows.h>

/* Usage

Writing:
   Registry_Class registry;

   if ( registry.Open_Key( "Software\\Program" ) )
   {
      // write int values
      uint32 data_is_right = 1;

      registry.Set_Value_Data_Int( "Value is Left", data_is_right );

      // write string values
      registry.Set_Value_Data_String( "Value is Left", "Data is Right" );
   }

Reading:
   Registry_Class registry;
      
   if ( registry.Open_Key( "Software\\Program" ) )
   {
      // read int values
      uint32 data = registry.Get_Value_Data_Int( "Value is Left", 0 ); //right is the default value to return

      // read string values
      char data_is_on_right_str[ DATA_LENGTH ];

      registry.Get_Value_Data_String( "The Value Is On Left", data_is_on_right_str, DATA_LENGTH, "Data if Value isn't found" );
   }
*/

class Registry_Class
{

   public:

            Registry_Class        ( HKEY root_key = HKEY_CURRENT_USER );
            ~Registry_Class       ( );
 
      void  Set_Root_Key          ( HKEY rootKey );
      BOOL  Create_Key            ( const char * key );
      BOOL  Open_Key              ( const char * key, REGSAM samDesired, BOOL createOnFail = TRUE );
      BOOL  Delete_Key            ( char *pKeyToDelete, BOOL deleteSubKeys/* = FALSE*/ );
      BOOL  Does_Key_Exist        ( char * key );
      void  Close_Key             ( void );
      int   Get_Key_Value_Count   ( void );
      BOOL  Get_Value_Data_String ( char * value, char * data, DWORD buffer_length, const char * default_data );
      BOOL  Get_Value_Data_Binary ( char * value, void * data, DWORD buffer_length, const BYTE * default_data );
      DWORD Get_Value_Data_Int    ( char * value, int default_data, BOOL * success = NULL );
      BOOL  Set_Value_Data_String ( char * value, const char * data );
      BOOL  Set_Value_Data_Int    ( char * value, DWORD data );
      BOOL  Set_Value_Data_Binary ( char * value, void * data, int size );
      BOOL  Delete_Value          ( char * value );

   private:
      HKEY  m_key_handle;
      HKEY  m_root_key;

};

#endif
