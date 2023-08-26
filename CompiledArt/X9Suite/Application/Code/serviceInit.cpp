
#include "precompiled.h"
#include "serviceInit.h"
#include "main.h"
#include <Sddl.h>
#include <Aclapi.h>

SERVICE_STATUS        ServiceInit::m_ServiceStatus;
SERVICE_STATUS_HANDLE ServiceInit::m_ServiceStatusHandle;

BOOL ServiceInit::ServiceInstall( void )
{
   BOOL success            = FALSE;
   SC_HANDLE scmHandle     = NULL;
   SC_HANDLE serviceHandle = NULL;

   do
   {
      // get the SCM Manager
      scmHandle = OpenSCManager( NULL, NULL, SC_MANAGER_CREATE_SERVICE );
      if ( !scmHandle )
      {
         ServiceDebugPrint( "Failed to open SCM.", GetLastError( ) );
         break;
      }

      // Create/Install the service
      serviceHandle = CreateService( scmHandle, 
                                     SERVICE_NAME, 
                                     SERVICE_NAME, 
                                     SC_MANAGER_CREATE_SERVICE, 
                                     SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS, 
                                     SERVICE_DEMAND_START,
                                     SERVICE_ERROR_IGNORE,
                                     GetCommandLine( ),
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     "" );
                                

      if ( !serviceHandle )
      {
         ServiceDebugPrint( "Failed to create service.", GetLastError( ) );
         break;
      }

      // set the description
      SERVICE_DESCRIPTION serviceDesc;
      serviceDesc.lpDescription = SERVICE_DESC_STR;

      if ( !ChangeServiceConfig2( serviceHandle, SERVICE_CONFIG_DESCRIPTION, &serviceDesc ) )
      {
         ServiceDebugPrint( "Failed to set service description.", GetLastError( ) );
         break;
      }
   
      CloseServiceHandle( serviceHandle );

      // re-open it with all access so we can change the security
      serviceHandle = OpenService( scmHandle, SERVICE_NAME, SERVICE_ALL_ACCESS );

      // set the security
      if ( !SetServiceSecurity( serviceHandle ) )
      {
         ServiceDebugPrint( "Failed to set service security.", GetLastError( ) );
         break;
      }

      success = TRUE;
   }
   while( 0 );

   if ( serviceHandle )
   {
      CloseServiceHandle( serviceHandle );
   }

   if ( scmHandle )
   {
      CloseServiceHandle( scmHandle );
   }

   return success;
}

BOOL ServiceInit::ReadServiceDACL( SC_HANDLE serviceHandle, char **ppDACLstring, ULONG *pStringSize )
{
   BOOL success = FALSE;
   ACL *pDacl   = NULL;
   SECURITY_DESCRIPTOR *pSecurityDescriptor = NULL;

   do
   {
      // see how many bytes we need
      DWORD bytesNeeded;
      if ( !QueryServiceObjectSecurity( serviceHandle, DACL_SECURITY_INFORMATION, NULL, 0, &bytesNeeded ) ) break;

      // allocate a security descriptor large enough
      DWORD size = bytesNeeded;
      pSecurityDescriptor = (SECURITY_DESCRIPTOR *)new char[ 256 ];

      if ( !QueryServiceObjectSecurity( serviceHandle, DACL_SECURITY_INFORMATION, pSecurityDescriptor, size, &bytesNeeded ) ) break;

      // extract the DACL
      BOOL daclPresent;
      BOOL daclDefaulted;
      if ( !GetSecurityDescriptorDacl( pSecurityDescriptor, &daclPresent, &pDacl, &daclDefaulted ) ) break;

      // convert it to string format
      if ( !ConvertSecurityDescriptorToStringSecurityDescriptor( pSecurityDescriptor, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, ppDACLstring, pStringSize ) ) break;

      success = TRUE;
   }
   while( 0 );

   if ( pDacl )
   {
      LocalFree( pDacl );
   }

   if ( pSecurityDescriptor )
   {
      delete [] pSecurityDescriptor;
   }

   return success;
}

BOOL ServiceInit::SetServiceSecurity( SC_HANDLE serviceHandle )
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

BOOL ServiceInit::ServiceUnInstall( void )
{
   BOOL success            = FALSE;
   SC_HANDLE scmHandle     = NULL;
   SC_HANDLE serviceHandle = NULL;

   do
   {
      // get the SCM Manager
      scmHandle = OpenSCManager( NULL, NULL, SC_MANAGER_CREATE_SERVICE );
      if ( !scmHandle )
      {
         ServiceDebugPrint( "Failed to open SCM.", GetLastError( ) );
         break;
      }
      
      serviceHandle = OpenService( scmHandle, SERVICE_NAME, STANDARD_RIGHTS_REQUIRED );
      if ( !serviceHandle )
      {
         ServiceDebugPrint( "Failed to open Service.", GetLastError( ) );
         break;
      }

      if ( !DeleteService( serviceHandle ) )
      {
         ServiceDebugPrint( "Failed to delete Service.", GetLastError( ) );
         break;
      }

      success = TRUE;
   }
   while( 0 );

   if ( serviceHandle )
   {
      CloseServiceHandle( serviceHandle );
   }

   if ( scmHandle )
   {
      CloseServiceHandle( scmHandle );
   }

   return success;
}

void ServiceInit::ServiceRun( void )
{
   // StartServiceCtrlDispatcher requires an array with NULL at the end so it knows the list of services is done.
   SERVICE_TABLE_ENTRY serviceTableEntry[] = 
   {
      { (LPTSTR) SERVICE_NAME, ServiceInit::ServiceStart },
      {          NULL        , NULL                      }
   };

   if ( !StartServiceCtrlDispatcher( serviceTableEntry ) )
   {
      ServiceDebugPrint( "Failed to start Service Control Dispatcher.", GetLastError( ) );
   }
}

void WINAPI ServiceInit::ServiceStart( DWORD argc, LPTSTR *argv )
{
   do
   {
      // note, beginning here, our service must update with the Service Control Manager at least once a second.
      memset( &m_ServiceStatus, 0, sizeof( m_ServiceStatus ) );

      m_ServiceStatusHandle = RegisterServiceCtrlHandler( (LPTSTR)SERVICE_NAME, ServiceInit::ServiceControlHandler );

      if ( (SERVICE_STATUS_HANDLE)0 == m_ServiceStatusHandle )
      {
         ServiceDebugPrint( "Failed to register Service Control Handler.", GetLastError( ) );
         break;
      }

      // now let the SCM know we're working on initializing
      m_ServiceStatus.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
      m_ServiceStatus.dwCurrentState     = SERVICE_START_PENDING;
      m_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
      m_ServiceStatus.dwWaitHint         = 500; //notify the SCM that we'll be done initing within 1/2 a second.

      SetServiceStatus( m_ServiceStatusHandle, &m_ServiceStatus );

      //Call our actual Initialize, which is generic across Service/Command style creation.
      BOOL result = g_Core.Initialize( argc, argv );

      if ( !result )
      {
         // if we fail, we need to let the SCM know.
         m_ServiceStatus.dwCurrentState = SERVICE_STOP;
         m_ServiceStatus.dwWaitHint     = 0;

         SetServiceStatus( m_ServiceStatusHandle, &m_ServiceStatus );
         break;
      }

      // we're ready to rock. Notify the SCM.
      m_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
      m_ServiceStatus.dwWaitHint     = 0;

      SetServiceStatus( m_ServiceStatusHandle, &m_ServiceStatus );

      // call the core app's tick, which will not return until the service is stopped.
      g_Core.Tick( );

      // Don't uninitialize, because the Core's deconstructor will

      m_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
      m_ServiceStatus.dwWaitHint     = 0;

      SetServiceStatus( m_ServiceStatusHandle, &m_ServiceStatus );
   }
   while( 0 );
}

void WINAPI ServiceInit::ServiceControlHandler( DWORD fdwControl )
{
   switch( fdwControl )
   {
      case SERVICE_CONTROL_STOP:
      {
         m_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
         m_ServiceStatus.dwWaitHint     = 500; //notify the SCM that we'll be done stopping within 1/2 a second.

         SetServiceStatus( m_ServiceStatusHandle, &m_ServiceStatus );

         break;
      }
   }
}

BOOL ServiceInit::ServiceCheckForShutdown( void )
{
   if ( SERVICE_STOP_PENDING == m_ServiceStatus.dwCurrentState )
   {
      return TRUE;
   }

   return FALSE;
}

void ServiceInit::ServiceDebugPrint( char *pDebugMsg, DWORD errorCode )
{
   char errorString[ 1024 ] = { 0 };
   _snprintf( errorString, 1022, "%s. Code %d\r\n", pDebugMsg, errorCode );
   
   char errorLogPath[ MAX_PATH ];
   sprintf( errorLogPath, "%s\\X9-errorLog.txt", g_Options.m_Options.databasePath );

   FILE *pFile = fopen( errorLogPath, "a" );

   if ( pFile )
   {
      fwrite( errorString, strlen( errorString ), 1, pFile );

      fclose( pFile );
   }
}
