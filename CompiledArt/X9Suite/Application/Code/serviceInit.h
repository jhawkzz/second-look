
#ifndef SERVICE_INIT_H_
#define SERVICE_INIT_H_

class ServiceInit
{
   public:
   
      static BOOL        ServiceInstall         ( void );
      static BOOL        ServiceUnInstall       ( void );
      static void        ServiceRun             ( void );                                                             //Our shell function to call all necessary Service calls
      static BOOL        SetServiceSecurity     ( SC_HANDLE serviceHandle );                                          //Used to set the DACL of the service.
      static BOOL        ReadServiceDACL        ( SC_HANDLE serviceHandle, char **ppDACLstring, ULONG *pStringSize ); //Get a string describing the current DACL
      static BOOL        ServiceCheckForShutdown( void );                                                             //Lets the main program know if the service should shut down.
      static void        ServiceDebugPrint      ( char *pDebugMsg, DWORD errorCode );                                 //Provides debug output when running as a service
      static void WINAPI ServiceStart           ( DWORD argc, LPTSTR *argv );                                         //Used to init the service with Windows
      static void WINAPI ServiceControlHandler  ( DWORD fdwControl );                                                 //Used to listen to the Service Control Manager for messages like Stop.

   public:

      static SERVICE_STATUS        m_ServiceStatus;
      static SERVICE_STATUS_HANDLE m_ServiceStatusHandle;
};

#endif
