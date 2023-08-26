
#pragma warning(disable : 4995) //Disable security warnings for stdio functions
#pragma warning(disable : 4996) //Disable security warnings for stdio functions
#define _CRT_SECURE_NO_DEPRECATE //Disable security warnings for stdio functions

#include <stdio.h>
#include <windows.h>
#include <psapi.h>
#include <Aclapi.h>
#include "..\\AppIntegrityDriver\\appIntegrityDriverDefines.h"

BOOL InstallDriver( const char *pInstallPath, BOOL forceStart );
BOOL UninstallDriver( BOOL forceStop );
BOOL SetServiceSecurity( SC_HANDLE serviceHandle );

int main( int argumentCount, char *pArgumentList[] )
{
   BOOL badParams = FALSE;

   if ( argumentCount < 2 )
   {
      badParams = TRUE;
   }
   else
   {
      if ( !_strcmpi( pArgumentList[ 1 ], "-install" ) )
      {
         if ( argumentCount < 3 )
         {
            printf( "Provide a path to install the driver.\n(Ex: dm -install C:\\Windows\\System32\\Drivers)\n" );
         }
         else
         {
            BOOL forceStart = FALSE;

            // check for no start
            if ( argumentCount > 3 && !_strcmpi( pArgumentList[ 3 ], "-forceStart" ) )
            {
               forceStart = TRUE;
            }

            InstallDriver( pArgumentList[ 2 ], forceStart );
         }
      }
      else if ( !_strcmpi( pArgumentList[ 1 ], "-uninstall" ) )
      {
         BOOL forceStop = FALSE;

         // check for force stop
         if ( argumentCount > 2 && !_strcmpi( pArgumentList[ 2 ], "-forceStop" ) )
         {
            forceStop = TRUE;
         }

         UninstallDriver( forceStop );
      }
      else
      {
         badParams = TRUE;
      }
   }

   if ( badParams )
   {
      printf( "DriverManager will install or uninstall AppIntegrityDriver.sys\r\n" );
      printf( "To Install: dm -install [PATH] [parameters]\r\n" );
      printf( "Install Parameters:\r\n" );
      printf( "\t-forceStart\tForces DM to automatically start the driver\n"
              "\t\t\tafter it is installed. This will allow the\n"
              "\t\t\tdriver to be completely installed without\n"
              "\t\t\trebooting, but has a rare chance to BSOD Windows.\r\n" );
      
      printf( "\n" );
      
      printf( "To Uninstall: dm -uninstall [parameters]\r\n" );
      printf( "Uninstall Parameters:\r\n" );
      printf( "\t-forceStop\tForces DM to stop the driver prior to uninstalling.\n"
              "\t\t\tThis will allow the driver to be completely removed\n"
              "\t\t\twithout rebooting, but has a rare chance to BSOD\n"
              "\t\t\tWindows.\r\n" );
   }

   return 0;
}

BOOL InstallDriver( const char *pInstallPath, BOOL forceStart )
{
   BOOL success        = FALSE;
   BOOL startedDriver  = FALSE;
   SC_HANDLE scmHandle = NULL;
   SC_HANDLE schDriver = NULL;

   do
   {
      printf( "Installing driver...\n" );

      // ---- open the SCM Manager ----
      scmHandle = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
      if ( !scmHandle )
      {
         printf( "\tERROR: Failed to open Service Control Manager. Be sure to run as administrator\n" );
         break;
      }

      // ---- create a unicode path to our driver ----
      char driverPath[ MAX_PATH ];

      // make sure there's a trailing \ on the install path.
      size_t strLen = strlen( pInstallPath );
      if ( pInstallPath[ strLen - 1 ] != '\\' )
      {
         sprintf( driverPath, "%s\\%s.sys", pInstallPath, DRIVER_NAME_NON_UNICODE );
      }
      else
      {
         sprintf( driverPath, "%s%s.sys", pInstallPath, DRIVER_NAME_NON_UNICODE );
      }
      
      WCHAR wDriverPath[ MAX_PATH ]; 
      MultiByteToWideChar( CP_ACP, 0, driverPath, -1, wDriverPath, MAX_PATH );

      // ---- make sure the file exists ----
      FILE *pFile = NULL;
      pFile = fopen( driverPath, "rb" );
      if ( !pFile )
      {
         printf( "\tERROR: %s doesn't exist\n", driverPath );
         break;
      }
      fclose( pFile );

      // ---- install our driver. All driver interaction requires unicode. ----
      schDriver = CreateServiceW( scmHandle,  
                                  DRIVER_NAME,
                                  DRIVER_DESC,
                                  SERVICE_ALL_ACCESS,
                                  SERVICE_KERNEL_DRIVER,
                                  SERVICE_SYSTEM_START,//Request that our driver boot with the system
                                  SERVICE_ERROR_NORMAL,
                                  wDriverPath,
			                         NULL,
			                         NULL,
			                         NULL,
			                         NULL,
			                         NULL );

      // did we fail to install?
      if ( !schDriver )
      {
         // that's ok if the error was that it's already installed.
         DWORD lastError = GetLastError( );
         if ( lastError != ERROR_SERVICE_EXISTS )
         {
            printf( "\tERROR: Could not install driver. Last Error: 0x%x\n", lastError );
            break;
         }
         else
         {
            printf( "\tDriver already installed\n" );
         }
      }
      else
      {
         printf( "\tDriver installed\n" );
      }

      // close/re-open the driver with proper permissions so we can set security
      CloseServiceHandle( schDriver );
      SC_HANDLE schDriver = OpenServiceW( scmHandle, DRIVER_NAME, SERVICE_ALL_ACCESS );

      // ---- set security ----
      printf( "\tSetting security permissions...\n" );
      BOOL result = SetServiceSecurity( schDriver );
      if ( result )
      {
         printf( "\tDriver security set\n" );
      }
      else
      {
         printf( "\tWARNING: Could not set driver security\n" );
      }

      // ---- try to start the driver. ----
      // It may already be running if it's already been installed. 
      // In that case, this does nothing.
      if ( forceStart )
      {
         printf( "\tAttempting to start driver...\n" );

         result = StartServiceW( schDriver, 0, NULL );

         // if we failed, that might be ok. It could already be running.
         if ( !result )
         {
            DWORD lastError = GetLastError( );
            if ( lastError != ERROR_SERVICE_ALREADY_RUNNING )
            {
               printf( "\tERROR: Could not start driver. Last Error: 0x%x\n", lastError );
               break;
            }
            else
            {
               printf( "\tDriver already running\n" );
            }
         }

         printf( "\tDriver started\n" );
         startedDriver = TRUE;
      }

      success = TRUE;
   }
   while( 0 );

   // it's installed, so now close the handle.
   if ( schDriver )
   {
      CloseServiceHandle( schDriver );
   }

   if ( scmHandle )
   {
      CloseServiceHandle( scmHandle );
   }

   if ( success )
   {
      // the success message should change based on whether the driver is running or not.
      if ( !startedDriver )
      {
         printf( "Successfully Installed Driver. Reboot to start the driver.\n" );
      }
      else
      {
         printf( "Successfully Installed Driver. Driver is running, no need to reboot.\n" );
      }
   }
   else
   {
      printf( "Failed to Install Driver\n" );
   }

   return success;
}

BOOL UninstallDriver( BOOL forceStop )
{
   BOOL success        = FALSE;
   BOOL driverStopped  = FALSE;
   HANDLE hToken       = NULL;
   char *pTokenUser    = NULL;
   SC_HANDLE scmHandle = NULL;
   SC_HANDLE schDriver = NULL;
   ACL *pNewDacl       = NULL;
   char *pPrevPriv     = NULL;

   do
   {
      printf( "Uninstalling Driver...\n" );

      // ---- adjust token privileges and lookup SID----
      printf( "\tElevating DM security priveleges...\n" );
      // get the token for this process
      if ( !OpenProcessToken( GetCurrentProcess( ), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken ) )
      {
         printf( "\tERROR: Could not elevate security privileges\n" );
         break;
      }

      // find the ownership privilege
      LUID luid;
      if ( !LookupPrivilegeValue(0, SE_TAKE_OWNERSHIP_NAME, &luid) )
      {
         printf( "\tERROR: Could not elevate security privileges\n" );
         break;
      }

      // adjust our privileges so ownership is enabled
      TOKEN_PRIVILEGES priv;
      priv.PrivilegeCount             = 1;
      priv.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;
      priv.Privileges[ 0 ].Luid       = luid;

      // get the current privilege info so we can restore it. We call twice to first get the buffer size, then the info
      DWORD prevPrivSize;
      AdjustTokenPrivileges( hToken, TokenPrivileges, &priv, 1, (TOKEN_PRIVILEGES *)&prevPrivSize, &prevPrivSize );
      
      DWORD error = GetLastError( );
      if ( error != ERROR_INSUFFICIENT_BUFFER )
      {
         printf( "\tERROR: Could not elevate security privileges\n" );
         break;
      }

      // adjust our privileges so ownership is enabled, and get the old state
      pPrevPriv = new char[ prevPrivSize ];
      if ( !AdjustTokenPrivileges( hToken, FALSE, &priv, prevPrivSize, (TOKEN_PRIVILEGES *)pPrevPriv, &prevPrivSize ) )
      {
         printf( "\tERROR: Could not elevate security privileges\n" );
         break;
      }

      // get the SID for our token. We call twice to first get the buffer size, then to get the info
      DWORD tokenInfoSize;
      GetTokenInformation( hToken, TokenUser, NULL, 0, &tokenInfoSize );

      error = GetLastError( );
      if ( error != ERROR_INSUFFICIENT_BUFFER )
      {
         printf( "\tERROR: Could not elevate security privileges\n" );
         break;
      }
      
      // get our SID
      pTokenUser = new char[ tokenInfoSize ];
      if ( !GetTokenInformation( hToken, TokenUser, pTokenUser, tokenInfoSize, &tokenInfoSize ) )
      {
         printf( "\tERROR: Could not elevate security privileges\n" );
         break;
      }

      printf( "\tPermissions elevated\n" );


      // ---- modify driver ----
      printf( "\tAdjusting driver security permissions...\n" );
      //We need to give ourselves permission to delete the driver.
      // this is a 3 step process. 
      //1. Open to set the owner, and make it us.
      //2. Open to set the DACL, and allow us permission to delete
      //3. Open to delete and delete.

      // now open the service manager
      scmHandle = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
      if ( !scmHandle )
      {
         printf( "\tERROR: Failed to open Service Control Manager. Be sure to run as administrator\n" );
         break;
      }
   
      // ---- take ownership of driver ----
      schDriver = OpenServiceW( scmHandle, DRIVER_NAME, WRITE_OWNER );
      if ( !schDriver )
      {
         printf( "\tERROR: Failed to open the driver service for write owner\n" );
         break;
      }

      //prepare a security descriptor for our service, with us as the owner
      SECURITY_DESCRIPTOR securityDescriptor;
      if ( !InitializeSecurityDescriptor( &securityDescriptor, SECURITY_DESCRIPTOR_REVISION ) )
      {
         printf( "\tERROR: Failed to create a security descriptor\n" );
         break;
      }

      // set our SID in the service security descriptor
      if ( !SetSecurityDescriptorOwner( &securityDescriptor, ((TOKEN_USER *)pTokenUser)->User.Sid, FALSE ) )
      {
         printf( "\tERROR: Failed to set our SID to security descriptor\n" );
         break;
      }

      // apply this SID to the service, making us an owner
      if ( !SetServiceObjectSecurity( schDriver, OWNER_SECURITY_INFORMATION, &securityDescriptor ) )
      {
         printf( "\tERROR: Failed to apply security descriptor to driver.\n" );
         break;
      }

      // close the service, we are now an owner.
      CloseServiceHandle( schDriver );


      // ---- set the DACL to give us permission to delete driver ----
      // open the driver for writing the DACL
      schDriver = OpenServiceW( scmHandle, DRIVER_NAME, WRITE_DAC );
      if ( !schDriver )
      {
         printf( "\tERROR: Failed to open the driver service for write dac\n" );
         break;
      }

      // create an ACE taht gives everyone (that's us!) permission to stop and delete
      EXPLICIT_ACCESS explicitAccess;
      BuildExplicitAccessWithName( &explicitAccess, TEXT("EVERYONE"), GENERIC_READ | SERVICE_START | SERVICE_STOP | READ_CONTROL | DELETE, SET_ACCESS, NO_INHERITANCE );

      // create a dacl
      if ( SetEntriesInAcl( 1, &explicitAccess, NULL, &pNewDacl ) != ERROR_SUCCESS )
      {
         printf( "\tERROR: Failed to set an ACE in new ACL\n" );
         break;
      }

      // re-init the security descriptor
      if ( !InitializeSecurityDescriptor( &securityDescriptor, SECURITY_DESCRIPTOR_REVISION ) )
      {
         printf( "\tERROR: Failed to create a security descriptor\n" );
         break;
      }

      // set the dacl in this descriptor
      if ( !SetSecurityDescriptorDacl( &securityDescriptor, TRUE, pNewDacl, FALSE ) )
      {
         printf( "\tERROR: Failed to set ACE in security descriptor DACL\n" );
         break;
      }

      // and set the dacl for the service. Cross your fingers!
      if ( !SetServiceObjectSecurity( schDriver, DACL_SECURITY_INFORMATION, &securityDescriptor ) )
      {
         printf( "\tERROR: Failed to apply DACL to driver service\n" );
         break;
      }

      //FINALLY, we should be able to close/open this driver and delete it.
      CloseServiceHandle( schDriver );
      printf( "\tDriver permissions adjusted\n" );

      
      // ---- stop the service ----
      // WARNING: we absolutely do not want to STOP the driver. That could 
      // mess up the hook chain and BSOD Windows. We just want to queue it for 
      // delete on the next reboot.
      // So while this is dangerous, it can be done with -forceStop
      if ( forceStop )
      {
         printf( "\tStopping driver...\n" );

         schDriver = OpenServiceW( scmHandle, DRIVER_NAME, SERVICE_STOP );
         if ( !schDriver )
         {
            printf( "\tERROR: Could not open driver service for stopping\n" );
            break;
         }

         SERVICE_STATUS serviceStatus = { 0 };
         if ( !ControlService( schDriver, SERVICE_CONTROL_STOP, &serviceStatus ) )
         {
            if ( serviceStatus.dwCurrentState != SERVICE_STOP_PENDING )
            {
               printf( "\tERROR: Failed to stop driver\n" );
               break;
            }
         }
         CloseServiceHandle( schDriver );
         printf( "\tDriver stopped\n" );

         driverStopped = TRUE;
      }

      // ---- delete the service ----
      printf( "\tDeleting driver...\n" );
      schDriver = OpenServiceW( scmHandle, DRIVER_NAME, DELETE );
      if ( !schDriver )
      {
         printf( "\tERROR: Failed to open driver service for delete\n" );
         break;
      }

      if ( !DeleteService( schDriver ) )
      {
         DWORD lastError = GetLastError( );

         // if we failed and it WASN'T marked for delete, that's bad
         if ( lastError != ERROR_SERVICE_MARKED_FOR_DELETE )
         {
            printf( "\tERROR: Failed to delete driver\n" );
            break;
         }
      }
         
      printf( "\tDriver deleted\n" );

      success = TRUE;
   }
   while( 0 );
   
   if ( pNewDacl )
   {
      LocalFree( pNewDacl );
   }

   if ( schDriver )
   {
      CloseServiceHandle( schDriver );
   }

   if ( scmHandle )
   {
      CloseServiceHandle( scmHandle );
   }

   if ( pTokenUser )
   {
      delete [] pTokenUser;
   }

   if ( pPrevPriv )
   {
      // restore previous privileges before deleting
      AdjustTokenPrivileges( hToken, FALSE, (TOKEN_PRIVILEGES *)pPrevPriv, 0, NULL, 0 );
      delete [] pPrevPriv;
   }

   if ( hToken )
   {
      CloseHandle( hToken );
   }

   if ( success )
   {
      if ( driverStopped )
      {
         printf( "Successully uninstalled driver. Driver is stopped, so no need to reboot\n" );
      }
      else
      {
         printf( "Successully uninstalled driver. Reboot to complete uninstallation\n" );
      }
   }
   else
   {
      printf( "Failed to Uninstall Driver\n" );
   }

   return success;
}

BOOL SetServiceSecurity( SC_HANDLE serviceHandle )
{
   BOOL success  = FALSE;
   ACL *pNewDacl = NULL;

   do
   {
      EXPLICIT_ACCESS explicitAccess[ 2 ];
      BuildExplicitAccessWithName( &explicitAccess[ 0 ], TEXT("SYSTEM"), SERVICE_ALL_ACCESS, SET_ACCESS, NO_INHERITANCE );
      
      BuildExplicitAccessWithName( &explicitAccess[ 1 ], TEXT("AUTHENTICATED USERS"), SERVICE_QUERY_CONFIG | 
                                                                                      SERVICE_QUERY_STATUS |
                                                                                      SERVICE_ENUMERATE_DEPENDENTS | 
                                                                                      SERVICE_START | 
                                                                                      SERVICE_INTERROGATE |
                                                                                      READ_CONTROL,

                                                                                      SET_ACCESS,
                                                                                      NO_INHERITANCE );
      // add future ACEs here

      if ( SetEntriesInAcl( 2, explicitAccess, NULL, &pNewDacl ) != ERROR_SUCCESS ) break;

      // create a brand new securityDescriptor
      SECURITY_DESCRIPTOR  securityDescriptor;
      if ( !InitializeSecurityDescriptor( &securityDescriptor, SECURITY_DESCRIPTOR_REVISION ) ) break;

      // apply our DACL to it
      if ( !SetSecurityDescriptorDacl( &securityDescriptor, TRUE, pNewDacl, FALSE ) ) break;

      // careful, this here is the real deal, and will apply the security dacl.
      if ( !SetServiceObjectSecurity( serviceHandle, DACL_SECURITY_INFORMATION, &securityDescriptor ) ) break;

      success = TRUE;
   }
   while( 0 );
   
   if ( pNewDacl )
   {
      LocalFree( pNewDacl );
   }

   return success;
}
