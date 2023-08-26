
#include <ntifs.h>
#include <ntddk.h>
#include <wdm.h>
#include <stdio.h>
#include <ntstrsafe.h>
#include <fltKernel.h>
#include <wchar.h>

#include "appIntegrityDriverDefines.h"
#include "hookSystemService.h"

// ---- debug switches ----

// don't do printouts in a free build. Only do them in checked.
#if DBG
#define DbgPrint DbgPrint
#else
#define DbgPrint
#endif

// ---- function pointers ----
//these are for our system service hooks
typedef NTSTATUS (NTAPI *pNtTerminateProcess)(IN HANDLE ProcessHandle OPTIONAL, IN NTSTATUS ExitStatus);

typedef NTSTATUS (NTAPI *pNtSetValueKey)( IN HANDLE KeyHandle, IN PUNICODE_STRING ValueName, IN ULONG TitleIndex OPTIONAL, IN ULONG Type, IN PVOID Data, IN ULONG DataSize );

typedef NTSTATUS (NTAPI *pNtDeleteValueKey)( IN HANDLE KeyHandle, IN PUNICODE_STRING ValueName );

typedef NTSTATUS (NTAPI *pNtDeleteKey)( IN HANDLE KeyHandle );

typedef NTSTATUS (NTAPI *pNtRenameKey)( IN HANDLE KeyHandle, IN PUNICODE_STRING ReplacementName );

typedef NTSTATUS (NTAPI *pNtOpenFile)( OUT PHANDLE           FileHandle,
                                       IN ACCESS_MASK        DesiredAccess,
                                       IN POBJECT_ATTRIBUTES ObjectAttributes,
                                       OUT PIO_STATUS_BLOCK  IoStatusBlock,
                                       IN ULONG              ShareAccess,
                                       IN ULONG              OpenOptions );

typedef NTSTATUS (NTAPI *pNtCreateFile)( OUT PHANDLE           FileHandle,
                                         IN ACCESS_MASK        DesiredAccess,
                                         IN POBJECT_ATTRIBUTES ObjectAttributes,
                                         OUT PIO_STATUS_BLOCK  IoStatusBlock,
                                         IN PLARGE_INTEGER     AllocationSize OPTIONAL,
                                         IN ULONG              FileAttributes,
                                         IN ULONG              ShareAccess,
                                         IN ULONG              CreateDisposition,
                                         IN ULONG              CreateOptions,
                                         IN PVOID              EaBuffer OPTIONAL,
                                         IN ULONG              EaLength );

//these are helper functions
typedef NTSTATUS (*pZwQueryInformationProcess) ( __in HANDLE ProcessHandle,
                                                 __in PROCESSINFOCLASS ProcessInformationClass,
                                                 __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
                                                 __in ULONG ProcessInformationLength,
                                                 __out_opt PULONG ReturnLength );

// ---- constants ----

// this defines how big our protection lists are
#define MAX_PROTECTED_REG_ITEMS (12)
#define MAX_PROTECTED_DIRS      (12)
#define MAX_PROTECTED_FILES     (12)

// define the reserved reg keys from WINREG.h
#define HKEY_CLASSES_ROOT   ((LONG)0x80000000)
#define HKEY_CURRENT_USER   ((LONG)0x80000001)
#define HKEY_LOCAL_MACHINE  ((LONG)0x80000002)

// define the process termination value from WINNT.h
#define PROCESS_TERMINATE (0x0001)

// this will be the altitude we request for our registry callback
#define REGISTRY_CALLBACK_ALTITUDE (L"320000")
#define PROCESS_CALLBACK_ALTITUDE  (L"320000")

// these define where NtTerminateProcess is on different OS platforms
const ULONG NtTerminateProcess_XPALL_callnumber    = 0x0101;
const ULONG NtTerminateProcess_VistaSP0_callnumber = 0x0152;
const ULONG NtTerminateProcess_VistaSP1_callnumber = 0x014E;
const ULONG NtTerminateProcess_VistaSP2_callnumber = 0x014E;
const ULONG NtTerminateProcess_Win7_callnumber     = 0x0172;

// these define where NtSetValueKey is on different OS platforms
const ULONG NtSetValueKey_XPALL_callnumber    = 0x00F7;
const ULONG NtSetValueKey_VistaSP0_callnumber = 0x0148;
const ULONG NtSetValueKey_VistaSP1_callnumber = 0x0144;
const ULONG NtSetValueKey_VistaSP2_callnumber = 0x0144;
const ULONG NtSetValueKey_Win7_callnumber     = 0x0166;

// these define where NtDeleteValueKey is on different OS platforms
const ULONG NtDeleteValueKey_XPALL_callnumber    = 0x0041;
const ULONG NtDeleteValueKey_VistaSP0_callnumber = 0x007E;
const ULONG NtDeleteValueKey_VistaSP1_callnumber = 0x007E;
const ULONG NtDeleteValueKey_VistaSP2_callnumber = 0x007E;
const ULONG NtDeleteValueKey_Win7_callnumber     = 0x006A;

// these define where NtDeleteKey is on different OS platforms
const ULONG NtDeleteKey_XPALL_callnumber    = 0x003F;
const ULONG NtDeleteKey_VistaSP0_callnumber = 0x007B;
const ULONG NtDeleteKey_VistaSP1_callnumber = 0x007B;
const ULONG NtDeleteKey_VistaSP2_callnumber = 0x007B;
const ULONG NtDeleteKey_Win7_callnumber     = 0x0067;

// these define where NtRenameKey is on different OS platforms
const ULONG NtRenameKey_XPALL_callnumber    = 0x00C0;
const ULONG NtRenameKey_VistaSP0_callnumber = 0x010B;
const ULONG NtRenameKey_VistaSP1_callnumber = 0x010B;
const ULONG NtRenameKey_VistaSP2_callnumber = 0x010B;
const ULONG NtRenameKey_Win7_callnumber     = 0x0122;

// these define where NtOpenFile is on different OS platforms
const ULONG NtOpenFile_XPALL_callnumber    = 0x0074;
const ULONG NtOpenFile_VistaSP0_callnumber = 0x00BA;
const ULONG NtOpenFile_VistaSP1_callnumber = 0x00BA;
const ULONG NtOpenFile_VistaSP2_callnumber = 0x00BA;
const ULONG NtOpenFile_Win7_callnumber     = 0x00B3;

// these define where NtCreateFile is on different OS platforms
const ULONG NtCreateFile_XPALL_callnumber    = 0x0025;
const ULONG NtCreateFile_VistaSP0_callnumber = 0x003C;
const ULONG NtCreateFile_VistaSP1_callnumber = 0x003C;
const ULONG NtCreateFile_VistaSP2_callnumber = 0x003C;
const ULONG NtCreateFile_Win7_callnumber     = 0x0042;

// ---- enums ----
// Enum for monitoring the system hook install state
typedef enum _SystemHookState
{
   SystemHookState_UnInstalled,
   SystemHookState_Installed
}
SystemHookState;

// ---- structs ----
// the name of the key or value the user wants protected.
typedef struct _PROTECTED_REG_ITEM
{
   BOOLEAN inUse;
   BOOLEAN isValue;                           //false if key, true if value
   BOOLEAN protectAllValues;                  //if a key and true, this will protect all values in key
   ULONG   rootClass;                         //hlm, hcu, etc
   WCHAR   keyPath[ MAX_REG_PATH_UNICODE ];   //the key path
   WCHAR   valueName[ MAX_REG_PATH_UNICODE ]; //the value name (if applicable)
}
PROTECTED_REG_ITEM;

// the path to the directory the user wants protected.
typedef struct _PROTECTED_DIR
{
   BOOLEAN inUse;
   WCHAR   dirPath[ MAX_WINDOWS_PATH_UNICODE ];
}
PROTECTED_DIR;

// the path to the directory the user wants protected.
typedef struct _PROTECTED_FILE
{
   BOOLEAN inUse;
   WCHAR   filePath[ MAX_WINDOWS_PATH_UNICODE ];
}
PROTECTED_FILE;

// a struct that will contain a parsed registry key that is similar to RegEdit's
typedef struct _USERSPACE_REG_KEY
{
   WCHAR name[ MAX_REG_PATH_UNICODE ];
   ULONG rootClass; //hlm, hcu, etc
}
USERSPACE_REG_KEY;

// this one is for system hooking.
typedef struct _SYSTEM_HOOK_INFO
{
   // ---- NtTerminateProcess Hook Members ----
   pNtTerminateProcess pOriginalNtTerminateProcess; //pointer to the original nt system service
   ULONG ntTerminateProcess_CallNumber; //the call number, since it's OS specific

   // ---- NtSetValueKey Hook Members ----
   pNtSetValueKey pOriginalNtSetValueKey; //pointer to the original nt system service
   ULONG ntSetValueKey_CallNumber; //the call number, since it's OS specific

   // ---- NtDeleteValueKey Hook Members ----
   pNtDeleteValueKey pOriginalNtDeleteValueKey; //pointer to the original nt system service
   ULONG ntDeleteValueKey_CallNumber; //the call number, since it's OS specific

   // ---- NtDeleteKey Hook Members ----
   pNtDeleteKey pOriginalNtDeleteKey; //pointer to the original nt system service
   ULONG ntDeleteKey_CallNumber; //the call number, since it's OS specific

   // ---- NtRenameKey Hook Members ----
   pNtRenameKey pOriginalNtRenameKey; //pointer to the original nt system service
   ULONG ntRenameKey_CallNumber; //the call number, since it's OS specific

   // ---- NtOpenFile Hook Members ----
   pNtOpenFile pOriginalNtOpenFile; //pointer to the original nt system service
   ULONG ntOpenFile_CallNumber; //the call number, since it's OS specific

   // ---- NtCreateFile Hook Members ----
   pNtCreateFile pOriginalNtCreateFile; //pointer to the original nt system service
   ULONG ntCreateFile_CallNumber; //the call number, since it's OS specific
   
   // ---- Hook Monitoring States ----
   // our state, this is whether the hook is installed or not
   SystemHookState systemHookState;
}
SYSTEM_HOOK_INFO;

// this structure contains everything required for our system callbacks
// which protect a process the same as our system hook would
typedef struct _SYSTEM_CALLBACK_INFO
{
   // ---- Reg Callback Members ----
   LARGE_INTEGER  regCallbackCookie;      //identifier for winnt
   UNICODE_STRING regCallbackAltitude;    //location in the priority for this callback
   BOOLEAN        regCallbackInstalled;   //true if our callback was installed

   // ---- Object Creation Callback Members ----
   VOID    *pObjRegistrationHandle; //handle provided by winnt
   BOOLEAN  objCallbackInstalled;   //true if our callback was installed

   // ---- Process Creation Callback Members ----
   BOOLEAN   psCreateNotifyCallbackInstalled; //true if our callback was installed

   // ---- Filter Manager Members ----
   PFLT_FILTER filterHandle; //handle provided to the filter manager
}
SYSTEM_CALLBACK_INFO;

typedef struct _APPINTEGRITY_ITEMS
{
   // bitfield defining which states we're monitoring
   ULONG monitorState;

   // ---- Reg Protection ----
   PROTECTED_REG_ITEM protectedRegItem[ MAX_PROTECTED_REG_ITEMS ]; //list of reg items to protect

   // ---- Dir Protection ----
   PROTECTED_DIR protectedDir[ MAX_PROTECTED_DIRS ]; //list of dirs to protect

   // ---- File Protection ----
   PROTECTED_FILE protectedFile[ MAX_PROTECTED_FILES ]; //list of files to protect

   // ---- Process Protection ----
   ULONG_PTR processId; // the ID of the process to protect
   PEPROCESS processObject; //the object of the process to protect
}
APPINTEGRITY_ITEMS;

// this is our standard driver extension data, which contains the above three
// and our driver's device object
typedef struct _DEVICE_EXTENSION
{
   DEVICE_OBJECT *pDeviceObject;
   
   SYSTEM_HOOK_INFO     systemHookInfo;
   SYSTEM_CALLBACK_INFO systemCallbackInfo;

   APPINTEGRITY_ITEMS appIntegrityItems;
}
DEVICE_EXTENSION;

// ---- prototypes ----
// standard for drivers
                                          DRIVER_INITIALIZE DriverEntry;
                                          DRIVER_UNLOAD     AppIntegrity_Unload_Driver;
__drv_dispatchType(IRP_MJ_CREATE)         DRIVER_DISPATCH   AppIntegrity_DispatchCreate;
__drv_dispatchType(IRP_MJ_CLOSE)          DRIVER_DISPATCH   AppIntegrity_DispatchClose;
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH   AppIntegrity_DispatchDeviceControl;

NTSTATUS DriverEntry( DRIVER_OBJECT *pDriverObject, PUNICODE_STRING registryPath );
void     AppIntegrity_Unload_Driver( DRIVER_OBJECT *pDriverObject );
NTSTATUS AppIntegrity_DispatchCreate( DEVICE_OBJECT *pDeviceObject, IRP *pIrp );
NTSTATUS AppIntegrity_DispatchClose( DEVICE_OBJECT *pDeviceObject, IRP *pIrp );
NTSTATUS AppIntegrity_DispatchDeviceControl( DEVICE_OBJECT *pDeviceObject, IRP *pIrp );

// utility
BOOLEAN  AppIntegrity_IsRegItemProtected( ULONG rootClass, PUNICODE_STRING pKeyPath, PUNICODE_STRING pValueName );
BOOLEAN  AppIntegrity_ParseRegistryKey( WCHAR *pRegistryKey, ULONG registryKeyLen, USERSPACE_REG_KEY *pParsedRegKey );

// managing system hook
NTSTATUS AppIntegrity_ActivateMonitoring( IRP *pIrp );
NTSTATUS AppIntegrity_DeactivateMonitoring( IRP *pIrp );
NTSTATUS AppIntegrity_SetProcessId( DEVICE_OBJECT *pDeviceObject, IRP *pIrp );
NTSTATUS AppIntegrity_SetProtectedRegItem( DEVICE_OBJECT *pDeviceObject, IRP *pIrp );
NTSTATUS AppIntegrity_ClearProtectedRegList( DEVICE_OBJECT *pDeviceObject, IRP *pIrp );
NTSTATUS AppIntegrity_SetProtectedDir( DEVICE_OBJECT *pDeviceObject, IRP *pIrp );
NTSTATUS AppIntegrity_ClearProtectedDirList( DEVICE_OBJECT *pDeviceObject, IRP *pIrp );
NTSTATUS AppIntegrity_SetProtectedFile( DEVICE_OBJECT *pDeviceObject, IRP *pIrp );
NTSTATUS AppIntegrity_ClearProtectedFileList( DEVICE_OBJECT *pDeviceObject, IRP *pIrp );
BOOLEAN  AppIntegrity_IsOSVistaSP0( void );
void     AppIntegrity_InstallSystemHook( BOOLEAN failOnPostVistaSP0 );
void     AppIntegrity_UnInstallSystemHook( void );
void     AppIntegrity_ApplyCallbacks( DRIVER_OBJECT *pDriverObject );
void     AppIntegrity_RemoveCallbacks( void );

NTSTATUS NTAPI NewNtTerminateProcess( IN HANDLE ProcessHandle OPTIONAL, IN NTSTATUS ExitStatus );

NTSTATUS NTAPI NewNtSetValueKey( IN HANDLE KeyHandle, IN PUNICODE_STRING ValueName, IN ULONG TitleIndex OPTIONAL, IN ULONG Type, IN PVOID Data, IN ULONG DataSize );

NTSTATUS NTAPI NewNtDeleteValueKey( IN HANDLE KeyHandle, IN PUNICODE_STRING ValueName );

NTSTATUS NTAPI NewNtDeleteKey( IN HANDLE KeyHandle );

NTSTATUS NTAPI NewNtRenameKey( IN HANDLE KeyHandle, IN PUNICODE_STRING ReplacementName );

NTSTATUS NTAPI NewNtOpenFile( OUT PHANDLE           FileHandle,
                              IN ACCESS_MASK        DesiredAccess,
                              IN POBJECT_ATTRIBUTES ObjectAttributes,
                              OUT PIO_STATUS_BLOCK  IoStatusBlock,
                              IN ULONG              ShareAccess,
                              IN ULONG              OpenOptions );

NTSTATUS NTAPI NewNtCreateFile( OUT PHANDLE           FileHandle,
                                IN ACCESS_MASK        DesiredAccess,
                                IN POBJECT_ATTRIBUTES ObjectAttributes,
                                OUT PIO_STATUS_BLOCK  IoStatusBlock,
                                IN PLARGE_INTEGER     AllocationSize OPTIONAL,
                                IN ULONG              FileAttributes,
                                IN ULONG              ShareAccess,
                                IN ULONG              CreateDisposition,
                                IN ULONG              CreateOptions,
                                IN PVOID              EaBuffer OPTIONAL,
                                IN ULONG              EaLength );

// ---- Registry Callback Functions ----
EX_CALLBACK_FUNCTION RegistryCallback;
NTSTATUS RegistryCallback( IN PVOID CallbackContext, OUT PVOID Argument1, OUT PVOID Argument2 );

// ---- Create Process Functions ----
OB_PREOP_CALLBACK_STATUS ObjectPreCallback( IN PVOID RegistrationContext, IN POB_PRE_OPERATION_INFORMATION OperationInformation );
VOID CreateProcessNotifyEx( __inout PEPROCESS               Process,
                            __in HANDLE                     ProcessId,
                            __in_opt PPS_CREATE_NOTIFY_INFO CreateInfo );

// ---- Filter Manager Functions ----
NTSTATUS FilterManagerUnload( __in FLT_FILTER_UNLOAD_FLAGS Flags );
FLT_PREOP_CALLBACK_STATUS PtPreOperationPassThrough( __inout PFLT_CALLBACK_DATA Data,
                                                     __in PCFLT_RELATED_OBJECTS FltObjects,
                                                     __deref_out_opt PVOID *CompletionContext );

CONST FLT_OPERATION_REGISTRATION FilterManagerCallbackList[] = //This defines the callback we want for each I/O operation below
{
    { IRP_MJ_CREATE,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_CLOSE,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_READ,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_WRITE,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_SET_INFORMATION,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_QUERY_EA,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_SET_EA,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_LOCK_CONTROL,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_CLEANUP,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_QUERY_SECURITY,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_SET_SECURITY,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_QUERY_QUOTA,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_SET_QUOTA,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_PNP,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_MDL_READ,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      PtPreOperationPassThrough,
      0 },

    { IRP_MJ_OPERATION_END }
};

// ---- globals ----
// a global pointer to our device so we can access it by callbacks
DEVICE_OBJECT  *g_pDeviceObject;

// ---- function definitions ----
NTSTATUS DriverEntry( DRIVER_OBJECT *pDriverObject, PUNICODE_STRING registryPath )
{
   DEVICE_OBJECT    *pDeviceObject;
   DEVICE_EXTENSION *pDeviceExtension;
   UNICODE_STRING    driverString;
   UNICODE_STRING    deviceString;
   NTSTATUS          ntStatus;

   DbgPrint( "Enter Func: DriverEntry\n" );

   // first setup our driver's name. We need a name so we can access it from usermode
   RtlInitUnicodeString( &driverString, L"\\Device\\"DRIVER_NAME );

   DbgPrint( "Task: IoCreateDevice\n" );
   // create our custom device, which the driver will manager.
   ntStatus = IoCreateDevice( pDriverObject, 
                              sizeof( DEVICE_EXTENSION ), 
                              &driverString, 
                              FILE_DEVICE_UNKNOWN, 
                              0, 
                              FALSE, 
                              &pDeviceObject );

   if ( ntStatus != STATUS_SUCCESS )
   {
      DbgPrint( "Failed: IoCreateDevice\n" );
      return ntStatus;
   }
   else
   {
      DbgPrint( "Success: IoCreateDevice\n" );
   }

   // get a pointer to the IO data allocated
   pDeviceExtension = pDeviceObject->DeviceExtension;
   
   // now create a device string so we can access this device in user mode.
   RtlInitUnicodeString( &deviceString, L"\\DosDevices\\"DRIVER_NAME );

   DbgPrint( "Task: IoCreateSymbolicLink\n" );
   // create a symbolic link so user-mode apps can access the io device.
   ntStatus = IoCreateSymbolicLink( &deviceString, &driverString );
   if ( ntStatus != STATUS_SUCCESS )
   {
      DbgPrint( "Failed: IoCreateSymbolicLink\n" );

      // cleanup if we failed here
      IoDeleteDevice( pDeviceObject );
      return ntStatus;
   }
   else
   {
      DbgPrint( "Success: IoCreateSymbolicLink\n" );
   }

   g_pDeviceObject = pDeviceObject;
   
   // clear out our protected items
   memset( &pDeviceExtension->appIntegrityItems, 0, sizeof( pDeviceExtension->appIntegrityItems ) );

   // clear our system hook struct
   memset( &pDeviceExtension->systemHookInfo, 0, sizeof( pDeviceExtension->systemHookInfo ) );

   // clear our system callback struct
   memset( &pDeviceExtension->systemCallbackInfo, 0, sizeof( pDeviceExtension->systemCallbackInfo ) );

   // prepare our driver functions
   pDriverObject->DriverUnload                         = AppIntegrity_Unload_Driver;
   pDriverObject->MajorFunction[IRP_MJ_CREATE]         = AppIntegrity_DispatchCreate;
   pDriverObject->MajorFunction[IRP_MJ_CLOSE]          = AppIntegrity_DispatchClose;
   pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AppIntegrity_DispatchDeviceControl;

   // initialize system callbacks for monitoring registry changes, process termination, etc
   AppIntegrity_ApplyCallbacks( pDriverObject );

   // install our hook
   AppIntegrity_InstallSystemHook( TRUE );
   
   return ntStatus;
}

void AppIntegrity_Unload_Driver( DRIVER_OBJECT *pDriverObject )
{
   UNICODE_STRING     deviceString;
   DEVICE_EXTENSION  *pExtension = pDriverObject->DeviceObject->DeviceExtension;
   NTSTATUS           ntStatus   = STATUS_DEVICE_CONFIGURATION_ERROR;

   DbgPrint( "Enter Func: Unload Driver\n" );

   // if we're being unloaded while our system hook is active, remove it.
   // NOTE: In normal usage, this driver should *NEVER* be removed after activating, 
   // because if someone hooks after us, they'll call our hook and BSOD Windows.
   if ( SystemHookState_Installed == pExtension->systemHookInfo.systemHookState )
   {
      AppIntegrity_UnInstallSystemHook( );
   }

   // remove our callbacks callback
   AppIntegrity_RemoveCallbacks( );
   
   // clean up our device
   DbgPrint( "Task: IoDeleteDevice\n" );
   IoDeleteDevice( pDriverObject->DeviceObject );
   DbgPrint( "Success: IoDeleteDevice\n" );

   // remove our symbolic link
   DbgPrint( "Task: IoDeleteSymbolicLink\n" );
   RtlInitUnicodeString( &deviceString, L"\\DosDevices\\"DRIVER_NAME );
   ntStatus = IoDeleteSymbolicLink( &deviceString );

   if ( STATUS_SUCCESS == ntStatus )
   {
      DbgPrint( "Success: IoDeleteSymbolicLink\n" );
   }
   else
   {
      DbgPrint( "Failed: IoDeleteSymbolicLink\n" );
   }
}

NTSTATUS AppIntegrity_DispatchCreate( DEVICE_OBJECT *pDeviceObject, IRP *pIrp )
{
   // initialize the IRP object.
   pIrp->IoStatus.Status      = STATUS_SUCCESS;
   pIrp->IoStatus.Information = 0;

   // let the IO Manager know we're done and the IRP is all his
   // use NO_INCREMENT because we don't need our thread priority boosted. We finished fast enough.
   IoCompleteRequest( pIrp, IO_NO_INCREMENT );

   return STATUS_SUCCESS;
}

NTSTATUS AppIntegrity_DispatchClose( DEVICE_OBJECT *pDeviceObject, IRP *pIrp )
{
   // initialize the IRP object.
   pIrp->IoStatus.Status      = STATUS_SUCCESS;
   pIrp->IoStatus.Information = 0;

   // let the IO Manager know we're done and the IRP is all his
   // use NO_INCREMENT because we don't need our thread priority boosted. We finished fast enough.
   IoCompleteRequest( pIrp, IO_NO_INCREMENT );

   return STATUS_SUCCESS;
}

NTSTATUS AppIntegrity_DispatchDeviceControl( DEVICE_OBJECT *pDeviceObject, IRP *pIrp )
{
   NTSTATUS           ntStatus  = STATUS_UNSUCCESSFUL;
   IO_STACK_LOCATION *pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

   switch( pIrpStack->Parameters.DeviceIoControl.IoControlCode )
   {
      // ---- system hook callback IOCTLs ----
      case IOCTL_APPINTEGRITY_ACTIVATE_MONITORING:
      {
         ntStatus = AppIntegrity_ActivateMonitoring( pIrp );
         break;
      }
      case IOCTL_APPINTEGRITY_DEACTIVATE_MONITORING:
      {
         ntStatus = AppIntegrity_DeactivateMonitoring( pIrp );
         break;
      }
      case IOCTL_APPINTEGRITY_SET_PROCESS_ID:
      {
         ntStatus = AppIntegrity_SetProcessId( pDeviceObject, pIrp );
         break;
      }
      case IOCTL_APPINTEGRITY_SET_PROTECTED_REG_ITEM:
      {
         ntStatus = AppIntegrity_SetProtectedRegItem( pDeviceObject, pIrp );
         break;
      }
      case IOCTL_APPINTEGRITY_CLEAR_PROTECTED_REG_LIST:
      {
         ntStatus = AppIntegrity_ClearProtectedRegList( pDeviceObject, pIrp );
         break;
      }
      case IOCTL_APPINTEGRITY_SET_PROTECTED_DIR:
      {
         ntStatus = AppIntegrity_SetProtectedDir( pDeviceObject, pIrp );
         break;
      }
      case IOCTL_APPINTEGRITY_CLEAR_PROTECTED_DIR_LIST:
      {
         ntStatus = AppIntegrity_ClearProtectedDirList( pDeviceObject, pIrp );
         break;
      }
      case IOCTL_APPINTEGRITY_SET_PROTECTED_FILE:
      {
         ntStatus = AppIntegrity_SetProtectedFile( pDeviceObject, pIrp );
         break;
      }
      case IOCTL_APPINTEGRITY_CLEAR_PROTECTED_FILE_LIST:
      {
         ntStatus = AppIntegrity_ClearProtectedFileList( pDeviceObject, pIrp );
         break;
      }
   }

   pIrp->IoStatus.Status = ntStatus;
   IoCompleteRequest( pIrp, IO_NO_INCREMENT );

   return ntStatus;
}

// ---- System Hook Functions ----
NTSTATUS AppIntegrity_ActivateMonitoring( IRP *pIrp )
{
   IO_STACK_LOCATION  *pIrpStack  = IoGetCurrentIrpStackLocation( pIrp );
   DEVICE_EXTENSION   *pExtension = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   DbgPrint( "Enter Func: AppIntegrity_ActivateMonitoring\n" );

   // make sure they sent the correct data size. We're going to read from their buffer, so it needs to be a size we support.
   if ( pIrpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof( ULONG ) )
   {
      DbgPrint( "Info: InputBufferLength Ok\n" );

      DbgPrint( "Info: Monitoring State Was: 0%x\n", pExtension->appIntegrityItems.monitorState );

      // get the info they passed in, and OR it to our bitfield
      pExtension->appIntegrityItems.monitorState |= *(ULONG *)pIrp->AssociatedIrp.SystemBuffer;

      DbgPrint( "Info: Monitoring State Is Now: 0%x\n", pExtension->appIntegrityItems.monitorState );
   }
   else
   {
      DbgPrint( "Info: InputBufferLength too small. Received: %d, Expected: %d\n", pIrpStack->Parameters.DeviceIoControl.InputBufferLength, sizeof( ULONG ) );
   }

   return STATUS_SUCCESS;
}

NTSTATUS AppIntegrity_DeactivateMonitoring( IRP *pIrp )
{
   IO_STACK_LOCATION  *pIrpStack  = IoGetCurrentIrpStackLocation( pIrp );
   DEVICE_EXTENSION   *pExtension = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   DbgPrint( "Enter Func: AppIntegrity_DeactivateMonitoring\n" );

   // make sure they sent the correct data size. We're going to read from their buffer, so it needs to be a size we support.
   if ( pIrpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof( ULONG ) )
   {
      DbgPrint( "Info: InputBufferLength Ok\n" );

      DbgPrint( "Info: Monitoring State Was: 0%x\n", pExtension->appIntegrityItems.monitorState );

      // get the info they passed in, and clear it from our bitfield
      pExtension->appIntegrityItems.monitorState &= ~(*(ULONG *)pIrp->AssociatedIrp.SystemBuffer);

      DbgPrint( "Info: Monitoring State Is Now: 0%x\n", pExtension->appIntegrityItems.monitorState );
   }
   else
   {
      DbgPrint( "Info: InputBufferLength too small. Received: %d, Expected: %d\n", pIrpStack->Parameters.DeviceIoControl.InputBufferLength, sizeof( ULONG ) );
   }

   return STATUS_SUCCESS;
}

NTSTATUS AppIntegrity_SetProcessId( DEVICE_OBJECT *pDeviceObject, IRP *pIrp )
{
   IO_STACK_LOCATION *pIrpStack  = IoGetCurrentIrpStackLocation( pIrp );
   DEVICE_EXTENSION  *pExtension = pDeviceObject->DeviceExtension;
   DbgPrint( "Enter Func: AppIntegrity_SetProcessId\n" );

   // make sure they sent the correct data size. We're going to read from their buffer, so it needs to be a size we support.
   if ( pIrpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof( ULONG ) )
   {
      pExtension->appIntegrityItems.processId = *(ULONG *)pIrp->AssociatedIrp.SystemBuffer;
      
      DbgPrint( "Success: App Integrity Process Id: %d\n", pExtension->appIntegrityItems.processId );
   }
   else
   {
      DbgPrint( "Info: InputBufferLength too small. Received: %d, Expected: %d\n", pIrpStack->Parameters.DeviceIoControl.InputBufferLength, sizeof( ULONG ) );
   }

   return STATUS_SUCCESS;
}

NTSTATUS AppIntegrity_SetProtectedRegItem( DEVICE_OBJECT *pDeviceObject, IRP *pIrp )
{
   IO_STACK_LOCATION       *pIrpStack     = IoGetCurrentIrpStackLocation( pIrp );
   DEVICE_EXTENSION        *pExtension    = pDeviceObject->DeviceExtension;
   PROTECTED_REG_ITEM_INFO *pItemInfo     = NULL;
   UNICODE_STRING           regItemString;
   UNICODE_STRING           searchKey;
   UNICODE_STRING           searchValue;
   ANSI_STRING              searchStringAnsi;
   int i;

   DbgPrint( "Enter Func: AppIntegrity_SetProtectedRegItem\n" );

   // make sure they sent the correct data size. We're going to read from their buffer, so it needs to be a size we support.
   if ( pIrpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof( PROTECTED_REG_ITEM_INFO ) )
   {
      // cast to an info struct, which is the usermode equivalent of reg item sturct
      pItemInfo = (PROTECTED_REG_ITEM_INFO *)pIrp->AssociatedIrp.SystemBuffer;

      pItemInfo->keyPath[ MAX_REG_PATH_UNICODE - 1 ]   = 0; //terminate for security reasons
      pItemInfo->valueName[ MAX_REG_PATH_UNICODE - 1 ] = 0; //terminate for security reasons

      // if we're adding, we can't be full
      if ( !pItemInfo->remove )
      {
         // find a free slot
         for ( i = 0; i < MAX_PROTECTED_REG_ITEMS; i++ )
         {
            if ( !pExtension->appIntegrityItems.protectedRegItem[ i ].inUse )
            {
               pExtension->appIntegrityItems.protectedRegItem[ i ].inUse            = TRUE;
               pExtension->appIntegrityItems.protectedRegItem[ i ].isValue          = pItemInfo->isValue;
               pExtension->appIntegrityItems.protectedRegItem[ i ].protectAllValues = pItemInfo->protectAllValues;
               pExtension->appIntegrityItems.protectedRegItem[ i ].rootClass        = pItemInfo->rootClass;

               RtlStringCchCopyNW( pExtension->appIntegrityItems.protectedRegItem[ i ].keyPath  , MAX_REG_PATH_UNICODE - 1, pItemInfo->keyPath  , MAX_REG_PATH_UNICODE - 1 );
               RtlStringCchCopyNW( pExtension->appIntegrityItems.protectedRegItem[ i ].valueName, MAX_REG_PATH_UNICODE - 1, pItemInfo->valueName, MAX_REG_PATH_UNICODE - 1 );
               break;
            }
         }
#if DBG
         if ( MAX_PROTECTED_REG_ITEMS == i )
         {
            DbgPrint( "Protect Reg Item List is full.\n" );
         }
         else
         {
            // convert to unicode, then to ansi, print it, then clean up
            RtlInitUnicodeString( &regItemString, pExtension->appIntegrityItems.protectedRegItem[ i ].keyPath );
            RtlUnicodeStringToAnsiString( &searchStringAnsi, &regItemString, TRUE );
            DbgPrint( "Success: Added Protected Reg Item: %s\\", searchStringAnsi.Buffer );
            RtlFreeAnsiString( &searchStringAnsi );

            RtlInitUnicodeString( &regItemString, pExtension->appIntegrityItems.protectedRegItem[ i ].valueName );
            RtlUnicodeStringToAnsiString( &searchStringAnsi, &regItemString, TRUE );
            DbgPrint( "%s\n", searchStringAnsi.Buffer );
            RtlFreeAnsiString( &searchStringAnsi );
         }
#endif
      }
      // if we're removing, we can't be empty
      else if ( pItemInfo->remove )
      {
         // make a unicode string out of the string passed in that we should remove
         RtlInitUnicodeString( &searchKey  , pItemInfo->keyPath );
         RtlInitUnicodeString( &searchValue, pItemInfo->valueName );

         // find the matching slot
         for ( i = 0; i < MAX_PROTECTED_REG_ITEMS; i++ )
         {
            if ( pExtension->appIntegrityItems.protectedRegItem[ i ].inUse )
            {
               // convert each item we have to a unicode string. We can blazenly overwrite the last
               // unicode string because it's just a wrapper struct that points to our buffer.
               RtlInitUnicodeString( &regItemString, pExtension->appIntegrityItems.protectedRegItem[ i ].keyPath );

               // does the key match?
               if ( !RtlCompareUnicodeString( &regItemString, &searchKey, TRUE ) )
               {
                  // and the value?
                  if ( !RtlCompareUnicodeString( &regItemString, &searchValue, TRUE ) )
                  {
                     pExtension->appIntegrityItems.protectedRegItem[ i ].inUse = FALSE;
                     break;
                  }
               }
            }
         }
#if DBG
         if ( MAX_PROTECTED_REG_ITEMS == i )
         {
            DbgPrint( "Failed to find Reg Item In List\n" );
         }
         else
         {
            DbgPrint( "Success: Removed Protected Reg Item\n" );
         }
#endif
      }
   }
   else
   {
      DbgPrint( "Info: InputBufferLength too small. Received: %d, Expected: %d\n", pIrpStack->Parameters.DeviceIoControl.InputBufferLength, sizeof( PROTECTED_REG_ITEM_INFO ) );
   }

   return STATUS_SUCCESS;
}

NTSTATUS AppIntegrity_ClearProtectedRegList( DEVICE_OBJECT *pDeviceObject, IRP *pIrp )
{
   DEVICE_EXTENSION *pExtension = pDeviceObject->DeviceExtension;
   int i;

   DbgPrint( "Enter Func: AppIntegrity_ClearProtectedRegList\n" );

   // free all the reg items we were monitoring.
   for ( i = 0; i < MAX_PROTECTED_REG_ITEMS; i++ )
   {
      pExtension->appIntegrityItems.protectedRegItem[ i ].inUse = FALSE;
   }
   
   return STATUS_SUCCESS;
}

NTSTATUS AppIntegrity_SetProtectedFile( DEVICE_OBJECT *pDeviceObject, IRP *pIrp )
{
   IO_STACK_LOCATION   *pIrpStack     = IoGetCurrentIrpStackLocation( pIrp );
   DEVICE_EXTENSION    *pExtension    = pDeviceObject->DeviceExtension;
   PROTECTED_FILE_INFO *pProtectedFileInfo;
   UNICODE_STRING       protectedFile;
   UNICODE_STRING       fileToRemove;
   ANSI_STRING          protectedFileAnsi;
   int i;

   DbgPrint( "Enter Func: AppIntegrity_SetProtectedFile\n" );

   // make sure they sent the correct data size. We're going to read from their buffer, so it needs to be a size we support.
   if ( pIrpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof( PROTECTED_FILE_INFO ) )
   {
      // cast to a PROTECTED_FILE_INFO pointer
      pProtectedFileInfo = (PROTECTED_FILE_INFO *)pIrp->AssociatedIrp.SystemBuffer;

      pProtectedFileInfo->filePath[ MAX_WINDOWS_PATH_UNICODE - 1 ] = 0; //terminate for security reasons

      // if we're adding, we can't be full
      if ( !pProtectedFileInfo->remove )
      {
         // find a free slot
         for ( i = 0; i < MAX_PROTECTED_FILES; i++ )
         {
            if ( !pExtension->appIntegrityItems.protectedFile[ i ].inUse )
            {
               pExtension->appIntegrityItems.protectedFile[ i ].inUse = TRUE;

               RtlStringCchCopyNW( pExtension->appIntegrityItems.protectedFile[ i ].filePath, MAX_WINDOWS_PATH_UNICODE - 1, pProtectedFileInfo->filePath, MAX_WINDOWS_PATH_UNICODE - 1 );

               // force to lower case, because we'll compare all strings in lower case in the system hook
               _wcslwr( pExtension->appIntegrityItems.protectedFile[ i ].filePath );
               break;
            }
         }
#if DBG
         if ( MAX_PROTECTED_FILES == i )
         {
            DbgPrint( "Protect File List is full.\n" );
         }
         else
         {
            // convert to unicode, then to ansi, print it, then clean up
            RtlInitUnicodeString( &protectedFile, pExtension->appIntegrityItems.protectedFile[ i ].filePath );
            RtlUnicodeStringToAnsiString( &protectedFileAnsi, &protectedFile, TRUE );
            DbgPrint( "Success: Added Protected File: %s", protectedFileAnsi.Buffer );
            RtlFreeAnsiString( &protectedFileAnsi );
         }
#endif
      }
      // if we're removing, we can't be empty
      else if ( pProtectedFileInfo->remove )
      {
         // make a unicode string out of the string passed in that we should remove
         RtlInitUnicodeString( &fileToRemove, pProtectedFileInfo->filePath );

         // find the matching slot
         for ( i = 0; i < MAX_PROTECTED_FILES; i++ )
         {
            if ( pExtension->appIntegrityItems.protectedFile[ i ].inUse )
            {
               // convert each item we have to a unicode string. We can blazenly overwrite the last
               // unicode string because it's just a wrapper struct that points to our buffer.
               RtlInitUnicodeString( &protectedFile, pExtension->appIntegrityItems.protectedFile[ i ].filePath );

               // does the file match?
               if ( !RtlCompareUnicodeString( &fileToRemove, &protectedFile, TRUE ) )
               {
                  pExtension->appIntegrityItems.protectedFile[ i ].inUse = FALSE;
                  break;
               }
            }
         }
#if DBG
         if ( MAX_PROTECTED_FILES == i )
         {
            DbgPrint( "Failed to find File Item In List\n" );
         }
         else
         {
            DbgPrint( "Success: Removed Protected File Item\n" );
         }
#endif
      }
   }
   else
   {
      DbgPrint( "Info: InputBufferLength too small. Received: %d, Expected: %d\n", pIrpStack->Parameters.DeviceIoControl.InputBufferLength, sizeof( PROTECTED_FILE_INFO ) );
   }

   return STATUS_SUCCESS;
}

NTSTATUS AppIntegrity_SetProtectedDir( DEVICE_OBJECT *pDeviceObject, IRP *pIrp )
{
   IO_STACK_LOCATION  *pIrpStack     = IoGetCurrentIrpStackLocation( pIrp );
   DEVICE_EXTENSION   *pExtension    = pDeviceObject->DeviceExtension;
   PROTECTED_DIR_INFO *pProtectedDirInfo;
   UNICODE_STRING      protectedDir;
   UNICODE_STRING      dirToRemove;
   ANSI_STRING         protectedDirAnsi;
   int i;

   DbgPrint( "Enter Func: AppIntegrity_SetProtectedDir\n" );

   // make sure they sent the correct data size. We're going to read from their buffer, so it needs to be a size we support.
   if ( pIrpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof( PROTECTED_DIR_INFO ) )
   {
      // cast to a PROTECTED_DIR_INFO pointer
      pProtectedDirInfo = (PROTECTED_DIR_INFO *)pIrp->AssociatedIrp.SystemBuffer;

      pProtectedDirInfo->dirPath[ MAX_WINDOWS_PATH_UNICODE - 1 ] = 0; //terminate for security reasons

      // if we're adding, we can't be full
      if ( !pProtectedDirInfo->remove )
      {
         // find a free slot
         for ( i = 0; i < MAX_PROTECTED_DIRS; i++ )
         {
            if ( !pExtension->appIntegrityItems.protectedDir[ i ].inUse )
            {
               pExtension->appIntegrityItems.protectedDir[ i ].inUse = TRUE;

               RtlStringCchCopyNW( pExtension->appIntegrityItems.protectedDir[ i ].dirPath, MAX_WINDOWS_PATH_UNICODE - 1, pProtectedDirInfo->dirPath, MAX_WINDOWS_PATH_UNICODE - 1 );

               // force to lower case, because we'll compare all strings in lower case in the system hook
               _wcslwr( pExtension->appIntegrityItems.protectedDir[ i ].dirPath );
               break;
            }
         }
#if DBG
         if ( MAX_PROTECTED_DIRS == i )
         {
            DbgPrint( "Protect Dir List is full.\n" );
         }
         else
         {
            // convert to unicode, then to ansi, print it, then clean up
            RtlInitUnicodeString( &protectedDir, pExtension->appIntegrityItems.protectedDir[ i ].dirPath );
            RtlUnicodeStringToAnsiString( &protectedDirAnsi, &protectedDir, TRUE );
            DbgPrint( "Success: Added Protected Dir: %s", protectedDirAnsi.Buffer );
            RtlFreeAnsiString( &protectedDirAnsi );
         }
#endif
      }
      // if we're removing, we can't be empty
      else if ( pProtectedDirInfo->remove )
      {
         // make a unicode string out of the string passed in that we should remove
         RtlInitUnicodeString( &dirToRemove, pProtectedDirInfo->dirPath );

         // find the matching slot
         for ( i = 0; i < MAX_PROTECTED_DIRS; i++ )
         {
            if ( pExtension->appIntegrityItems.protectedDir[ i ].inUse )
            {
               // convert each item we have to a unicode string. We can blazenly overwrite the last
               // unicode string because it's just a wrapper struct that points to our buffer.
               RtlInitUnicodeString( &protectedDir, pExtension->appIntegrityItems.protectedDir[ i ].dirPath );

               // does the dir match?
               if ( !RtlCompareUnicodeString( &dirToRemove, &protectedDir, TRUE ) )
               {
                  pExtension->appIntegrityItems.protectedDir[ i ].inUse = FALSE;
                  break;
               }
            }
         }
#if DBG
         if ( MAX_PROTECTED_DIRS == i )
         {
            DbgPrint( "Failed to find Dir Item In List\n" );
         }
         else
         {
            DbgPrint( "Success: Removed Protected Dir Item\n" );
         }
#endif
      }
   }
   else
   {
      DbgPrint( "Info: InputBufferLength too small. Received: %d, Expected: %d\n", pIrpStack->Parameters.DeviceIoControl.InputBufferLength, sizeof( PROTECTED_DIR_INFO ) );
   }

   return STATUS_SUCCESS;
}

NTSTATUS AppIntegrity_ClearProtectedFileList( DEVICE_OBJECT *pDeviceObject, IRP *pIrp )
{
   DEVICE_EXTENSION *pExtension = pDeviceObject->DeviceExtension;
   int i;

   DbgPrint( "Enter Func: AppIntegrity_ClearProtectedFileList\n" );

   // free all the file items we were monitoring.
   for ( i = 0; i < MAX_PROTECTED_FILES; i++ )
   {
      pExtension->appIntegrityItems.protectedFile[ i ].inUse = FALSE;
   }
   
   return STATUS_SUCCESS;
}

NTSTATUS AppIntegrity_ClearProtectedDirList( DEVICE_OBJECT *pDeviceObject, IRP *pIrp )
{
   DEVICE_EXTENSION *pExtension = pDeviceObject->DeviceExtension;
   int i;

   DbgPrint( "Enter Func: AppIntegrity_ClearProtectedDirList\n" );

   // free all the dir items we were monitoring.
   for ( i = 0; i < MAX_PROTECTED_DIRS; i++ )
   {
      pExtension->appIntegrityItems.protectedDir[ i ].inUse = FALSE;
   }
   
   return STATUS_SUCCESS;
}

void AppIntegrity_ApplyCallbacks( DRIVER_OBJECT *pDriverObject )
{
// don't define the callback stuff on pre-vista. It isn't supported.
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
   DEVICE_EXTENSION          *pExtension = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   FLT_REGISTRATION          filterRegInfo = { 0 };
   OB_OPERATION_REGISTRATION obOperationReg;
   OB_CALLBACK_REGISTRATION  obCallbackReg;
   NTSTATUS                  ntStatus;

   // assume we have NOT installed the callbacks
   pExtension->systemCallbackInfo.regCallbackInstalled            = FALSE;
   pExtension->systemCallbackInfo.objCallbackInstalled            = FALSE;
   pExtension->systemCallbackInfo.psCreateNotifyCallbackInstalled = FALSE;

   // setup our registry callback
   RtlInitUnicodeString( &pExtension->systemCallbackInfo.regCallbackAltitude, REGISTRY_CALLBACK_ALTITUDE );
   
   ntStatus = CmRegisterCallbackEx( RegistryCallback, 
                                    &pExtension->systemCallbackInfo.regCallbackAltitude, 
                                    g_pDeviceObject, 
                                    NULL, 
                                    &pExtension->systemCallbackInfo.regCallbackCookie, 
                                    NULL );

   if ( STATUS_SUCCESS == ntStatus )
   {
      pExtension->systemCallbackInfo.regCallbackInstalled = TRUE;
   }
   else
   {
      DbgPrint( "CmRegisterCallbackEx failed: 0x%x\n", ntStatus );
   }

   // setup our process creation callback
   ntStatus = PsSetCreateProcessNotifyRoutineEx( CreateProcessNotifyEx, FALSE );

   if ( STATUS_SUCCESS == ntStatus )
   {
      pExtension->systemCallbackInfo.psCreateNotifyCallbackInstalled = TRUE;

      // now setup our object creation callback
      obOperationReg.ObjectType    = PsProcessType;
      obOperationReg.Operations    = OB_OPERATION_HANDLE_CREATE;
      obOperationReg.PreOperation  = ObjectPreCallback;
      obOperationReg.PostOperation = NULL;
      
      RtlInitUnicodeString( &obCallbackReg.Altitude, PROCESS_CALLBACK_ALTITUDE );

      obCallbackReg.Version                    = OB_FLT_REGISTRATION_VERSION;
      obCallbackReg.OperationRegistrationCount = 1;
      obCallbackReg.RegistrationContext        = g_pDeviceObject;
      obCallbackReg.OperationRegistration      = &obOperationReg;

      ntStatus = ObRegisterCallbacks( &obCallbackReg, &pExtension->systemCallbackInfo.pObjRegistrationHandle );

      if ( STATUS_SUCCESS == ntStatus )
      {
         pExtension->systemCallbackInfo.objCallbackInstalled = TRUE;
      }
      else
      {
         DbgPrint( "ObRegisterCallback failed: 0x%x\n", ntStatus );
      }
   }
   else
   {
      DbgPrint( "PsSetCreateProcessNotifyRoutineEx failed: 0x%x\n", ntStatus );
   }

   // setup our I/O filter callback
   filterRegInfo.Size                  = sizeof( filterRegInfo );
   filterRegInfo.Version               = FLT_REGISTRATION_VERSION;
   filterRegInfo.Flags                 = 0;
   filterRegInfo.ContextRegistration   = NULL;
   filterRegInfo.OperationRegistration = FilterManagerCallbackList;
   filterRegInfo.FilterUnloadCallback  = FilterManagerUnload;

   ntStatus = FltRegisterFilter( pDriverObject, &filterRegInfo, &pExtension->systemCallbackInfo.filterHandle );
   DbgPrint( "FltRegisterFilter status: 0x%x\n", ntStatus );

   if ( STATUS_SUCCESS == ntStatus )
   {
      ntStatus = FltStartFiltering( pExtension->systemCallbackInfo.filterHandle );
      DbgPrint( "FltStartFiltering status: 0x%x\n", ntStatus );
   }
#endif //#if (NTDDI_VERSION >= NTDDI_LONGHORN)
}

void AppIntegrity_RemoveCallbacks( void )
{
// don't define the callback stuff on pre-vista. It isn't supported.
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
   DEVICE_EXTENSION *pExtension = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;

   // remove the object creation callback
   if ( pExtension->systemCallbackInfo.objCallbackInstalled )
   {
      ObUnRegisterCallbacks( pExtension->systemCallbackInfo.pObjRegistrationHandle );
   }

   // remove the ps creation callback
   if ( pExtension->systemCallbackInfo.psCreateNotifyCallbackInstalled )
   {
      PsSetCreateProcessNotifyRoutineEx( CreateProcessNotifyEx, TRUE );
   }

   // and the registry callback
   if ( pExtension->systemCallbackInfo.regCallbackInstalled )
   {
      CmUnRegisterCallback( pExtension->systemCallbackInfo.regCallbackCookie );
   }
#endif //#if (NTDDI_VERSION >= NTDDI_LONGHORN)
}

BOOLEAN AppIntegrity_IsOSVistaSP0( void )
{
   RTL_OSVERSIONINFOEXW osVersionInfo = { 0 };

   // get the OS version
   osVersionInfo.dwOSVersionInfoSize = sizeof( osVersionInfo );
   RtlGetVersion( (RTL_OSVERSIONINFOW *)&osVersionInfo );

   // this is the version set for Vista SP0
   if ( 6 == osVersionInfo.dwMajorVersion &&
        0 == osVersionInfo.dwMinorVersion &&
        0 == osVersionInfo.wServicePackMajor )
   {
      return TRUE;
   }

   return FALSE;
}

void AppIntegrity_InstallSystemHook( BOOLEAN failOnPostVistaSP0 )
{
   DEVICE_EXTENSION    *pExtension                = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   RTL_OSVERSIONINFOEXW osVersionInfo             = { 0 };
   BOOLEAN              windowsVersionUnsupported = FALSE;

   // ---- determine which version of Windows we're running ----
   osVersionInfo.dwOSVersionInfoSize = sizeof( osVersionInfo );
   RtlGetVersion( (RTL_OSVERSIONINFOW *)&osVersionInfo );

   // ---- test for post Vista SP0 ----
   if ( osVersionInfo.dwMajorVersion >= 6 &&
        VER_NT_WORKSTATION == osVersionInfo.wProductType )
   {
      // we know we're Vista or greater. Now just fail if it _is_ VistaSP0
      if ( !AppIntegrity_IsOSVistaSP0( ) )
      {
         // since this is newer than Vista SP0, then callbacks are supported, so we don't need to hook.
         if ( failOnPostVistaSP0 )
         {
            windowsVersionUnsupported = TRUE;
            DbgPrint( "Windows Version is newer than Vista SP0. Not hooking because callbacks are supported.\n" );
         }
      }
   }

   // ---- attempt to set the proper OS call number ----
   // Windows 7
   if ( 6 == osVersionInfo.dwMajorVersion &&
        1 == osVersionInfo.dwMinorVersion && 
        VER_NT_WORKSTATION == osVersionInfo.wProductType )
   {
      // service pack 0, or no service pack
      if ( 0 == osVersionInfo.wServicePackMajor )
      {
         DbgPrint( "Windows Version is Windows 7 SP0\n" );
         pExtension->systemHookInfo.ntTerminateProcess_CallNumber = NtTerminateProcess_Win7_callnumber;
         pExtension->systemHookInfo.ntSetValueKey_CallNumber      = NtSetValueKey_Win7_callnumber;
         pExtension->systemHookInfo.ntDeleteValueKey_CallNumber   = NtDeleteValueKey_Win7_callnumber;
         pExtension->systemHookInfo.ntDeleteKey_CallNumber        = NtDeleteKey_Win7_callnumber;
         pExtension->systemHookInfo.ntRenameKey_CallNumber        = NtRenameKey_Win7_callnumber;
         pExtension->systemHookInfo.ntOpenFile_CallNumber         = NtOpenFile_Win7_callnumber;
         pExtension->systemHookInfo.ntCreateFile_CallNumber       = NtCreateFile_Win7_callnumber;
      }
      else
      {
         DbgPrint( "Unknown Windows Version of Windows 7. Service Pack Is: M:%d m:%d\n", osVersionInfo.wServicePackMajor, osVersionInfo.wServicePackMinor );
         windowsVersionUnsupported = TRUE;
      }
   }
   // Windows Vista
   else if ( 6 == osVersionInfo.dwMajorVersion &&
             0 == osVersionInfo.dwMinorVersion && 
        	    VER_NT_WORKSTATION == osVersionInfo.wProductType )
   {
      // service pack 0, or no service pack
      if ( 0 == osVersionInfo.wServicePackMajor )
      {
         DbgPrint( "Windows Version is Vista SP0\n" );
         pExtension->systemHookInfo.ntTerminateProcess_CallNumber = NtTerminateProcess_VistaSP0_callnumber;
         pExtension->systemHookInfo.ntSetValueKey_CallNumber      = NtSetValueKey_VistaSP0_callnumber;
         pExtension->systemHookInfo.ntDeleteValueKey_CallNumber   = NtDeleteValueKey_VistaSP0_callnumber;
         pExtension->systemHookInfo.ntDeleteKey_CallNumber        = NtDeleteKey_VistaSP0_callnumber;
         pExtension->systemHookInfo.ntRenameKey_CallNumber        = NtRenameKey_VistaSP0_callnumber;
         pExtension->systemHookInfo.ntOpenFile_CallNumber         = NtOpenFile_VistaSP0_callnumber;
         pExtension->systemHookInfo.ntCreateFile_CallNumber       = NtCreateFile_VistaSP0_callnumber;
      }
      else if ( 1 == osVersionInfo.wServicePackMajor )
      {
         DbgPrint( "Windows Version is Vista SP1\n" );
         pExtension->systemHookInfo.ntTerminateProcess_CallNumber = NtTerminateProcess_VistaSP1_callnumber;
         pExtension->systemHookInfo.ntSetValueKey_CallNumber      = NtSetValueKey_VistaSP1_callnumber;
         pExtension->systemHookInfo.ntDeleteValueKey_CallNumber   = NtDeleteValueKey_VistaSP1_callnumber;
         pExtension->systemHookInfo.ntDeleteKey_CallNumber        = NtDeleteKey_VistaSP1_callnumber;
         pExtension->systemHookInfo.ntRenameKey_CallNumber        = NtRenameKey_VistaSP1_callnumber;
         pExtension->systemHookInfo.ntOpenFile_CallNumber         = NtOpenFile_VistaSP1_callnumber;
         pExtension->systemHookInfo.ntCreateFile_CallNumber       = NtCreateFile_VistaSP1_callnumber;
      }
      else if ( 2 == osVersionInfo.wServicePackMajor )
      {
         DbgPrint( "Windows Version is Vista SP2\n" );
         pExtension->systemHookInfo.ntTerminateProcess_CallNumber = NtTerminateProcess_VistaSP2_callnumber;
         pExtension->systemHookInfo.ntSetValueKey_CallNumber      = NtSetValueKey_VistaSP2_callnumber;
         pExtension->systemHookInfo.ntDeleteValueKey_CallNumber   = NtDeleteValueKey_VistaSP2_callnumber;
         pExtension->systemHookInfo.ntDeleteKey_CallNumber        = NtDeleteKey_VistaSP2_callnumber;
         pExtension->systemHookInfo.ntRenameKey_CallNumber        = NtRenameKey_VistaSP2_callnumber;
         pExtension->systemHookInfo.ntOpenFile_CallNumber         = NtOpenFile_VistaSP2_callnumber;
         pExtension->systemHookInfo.ntCreateFile_CallNumber       = NtCreateFile_VistaSP2_callnumber;
      }
      else
      {
         DbgPrint( "Unknown Windows Version of Windows Vista. Service Pack Is: M:%d m:%d\n", osVersionInfo.wServicePackMajor, osVersionInfo.wServicePackMinor );
         windowsVersionUnsupported = TRUE;
      }
   }
   // Windows XP
   else if ( 5 == osVersionInfo.dwMajorVersion &&
             1 == osVersionInfo.dwMinorVersion )
   {
      // no service pack checks needed
      DbgPrint( "Windows Version is XP\n" );
      pExtension->systemHookInfo.ntTerminateProcess_CallNumber = NtTerminateProcess_XPALL_callnumber;
      pExtension->systemHookInfo.ntSetValueKey_CallNumber      = NtSetValueKey_XPALL_callnumber;
      pExtension->systemHookInfo.ntDeleteValueKey_CallNumber   = NtDeleteValueKey_XPALL_callnumber;
      pExtension->systemHookInfo.ntDeleteKey_CallNumber        = NtDeleteKey_XPALL_callnumber;
      pExtension->systemHookInfo.ntRenameKey_CallNumber        = NtRenameKey_XPALL_callnumber;
      pExtension->systemHookInfo.ntOpenFile_CallNumber         = NtOpenFile_XPALL_callnumber;
      pExtension->systemHookInfo.ntCreateFile_CallNumber       = NtCreateFile_XPALL_callnumber;
   }
   // Unsupported version of Windows
   else
   {
      DbgPrint( "Windows Version is Unknown! Version Is: M:%d m:%d Service Pack M:%d m:%d\n", osVersionInfo.dwMajorVersion, 
                                                                                              osVersionInfo.dwMinorVersion,
                                                                                              osVersionInfo.wServicePackMajor,
                                                                                              osVersionInfo.wServicePackMinor );
      windowsVersionUnsupported = TRUE;
   }

   // If we didn't find an exact OS match with what we support, don't hook.
   if ( windowsVersionUnsupported )
   {
      pExtension->systemHookInfo.ntTerminateProcess_CallNumber = 0;
      pExtension->systemHookInfo.ntSetValueKey_CallNumber      = 0;
      pExtension->systemHookInfo.ntDeleteValueKey_CallNumber   = 0;
      pExtension->systemHookInfo.ntDeleteKey_CallNumber        = 0;
      pExtension->systemHookInfo.ntRenameKey_CallNumber        = 0;
      pExtension->systemHookInfo.ntOpenFile_CallNumber         = 0;
      pExtension->systemHookInfo.ntCreateFile_CallNumber       = 0;

      DbgPrint( "Info: Windows version unsupported. No hooks will be installed.\n" );
   }
   else
   {
      // ---- hook system services ----
      // hook NtTerminateProcess
      if ( pExtension->systemHookInfo.ntTerminateProcess_CallNumber )
      {
         // hook the system service so we get ours called
         pExtension->systemHookInfo.pOriginalNtTerminateProcess = (PVOID)HookNtSystemService( pExtension->systemHookInfo.ntTerminateProcess_CallNumber, 
                                                                                             (ULONG) NewNtTerminateProcess );

         DbgPrint( "NtTerminateProcess Hook Installed!\n" );
         DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtTerminateProcess );
         DbgPrint( "Info: New      Ptr: %x\n", (ULONG) NewNtTerminateProcess );
      }
      else
      {
         DbgPrint( "Info: Cannot enable NtTerminateProcess Hook. You need to set the call number!\n" );
      }

      // hook NtSetValueKey
      if ( pExtension->systemHookInfo.ntSetValueKey_CallNumber )
      {
         // hook the system service so we get ours called
         pExtension->systemHookInfo.pOriginalNtSetValueKey = (PVOID)HookNtSystemService( pExtension->systemHookInfo.ntSetValueKey_CallNumber, 
                                                                                        (ULONG) NewNtSetValueKey );

         DbgPrint( "NtSetValueKey Hook Installed!\n" );
         DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtSetValueKey );
         DbgPrint( "Info: New      Ptr: %x\n", (ULONG) NewNtSetValueKey );
      }
      else
      {
         DbgPrint( "Info: Cannot enable NtSetValueKey Hook. You need to set the call number!\n" );
      }

      // hook NtDeleteValueKey
      if ( pExtension->systemHookInfo.ntDeleteValueKey_CallNumber )
      {
         // hook the system service so we get ours called
         pExtension->systemHookInfo.pOriginalNtDeleteValueKey = (PVOID)HookNtSystemService( pExtension->systemHookInfo.ntDeleteValueKey_CallNumber, 
                                                                                           (ULONG) NewNtDeleteValueKey );

         DbgPrint( "NtDeleteValueKey Hook Installed!\n" );
         DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtDeleteValueKey );
         DbgPrint( "Info: New      Ptr: %x\n", (ULONG) NewNtDeleteValueKey );
      }
      else
      {
         DbgPrint( "Info: Cannot enable NtDeleteValueKey Hook. You need to set the call number!\n" );
      }

      // hook NtDeleteKey
      if ( pExtension->systemHookInfo.ntDeleteKey_CallNumber )
      {
         // hook the system service so we get ours called
         pExtension->systemHookInfo.pOriginalNtDeleteKey = (PVOID)HookNtSystemService( pExtension->systemHookInfo.ntDeleteKey_CallNumber, 
                                                                                      (ULONG) NewNtDeleteKey );

         DbgPrint( "NtDeleteKey Hook Installed!\n" );
         DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtDeleteKey );
         DbgPrint( "Info: New      Ptr: %x\n", (ULONG) NewNtDeleteKey );
      }
      else
      {
         DbgPrint( "Info: Cannot enable NtDeleteKey Hook. You need to set the call number!\n" );
      }

      // hook NtRenameKey
      if ( pExtension->systemHookInfo.ntRenameKey_CallNumber )
      {
         // hook the system service so we get ours called
         pExtension->systemHookInfo.pOriginalNtRenameKey = (PVOID)HookNtSystemService( pExtension->systemHookInfo.ntRenameKey_CallNumber, 
                                                                                      (ULONG) NewNtRenameKey );

         DbgPrint( "NtRenameKey Hook Installed!\n" );
         DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtRenameKey );
         DbgPrint( "Info: New      Ptr: %x\n", (ULONG) NewNtRenameKey );
      }
      else
      {
         DbgPrint( "Info: Cannot enable NtRenameKey Hook. You need to set the call number!\n" );
      }

      // hook NtOpenFile
      if ( pExtension->systemHookInfo.ntOpenFile_CallNumber )
      {
         // hook the system service so we get ours called
         pExtension->systemHookInfo.pOriginalNtOpenFile = (PVOID)HookNtSystemService( pExtension->systemHookInfo.ntOpenFile_CallNumber, 
                                                                                      (ULONG) NewNtOpenFile );

         DbgPrint( "NtOpenFile Hook Installed!\n" );
         DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtOpenFile );
         DbgPrint( "Info: New      Ptr: %x\n", (ULONG) NewNtOpenFile );
      }
      else
      {
         DbgPrint( "Info: Cannot enable NtOpenFile Hook. You need to set the call number!\n" );
      }

      // hook NtCreateFile
      if ( pExtension->systemHookInfo.ntCreateFile_CallNumber )
      {
         // hook the system service so we get ours called
         pExtension->systemHookInfo.pOriginalNtCreateFile = (PVOID)HookNtSystemService( pExtension->systemHookInfo.ntCreateFile_CallNumber, 
                                                                                        (ULONG) NewNtCreateFile );

         DbgPrint( "NtCreateFile Hook Installed!\n" );
         DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtCreateFile );
         DbgPrint( "Info: New      Ptr: %x\n", (ULONG) NewNtCreateFile );
      }
      else
      {
         DbgPrint( "Info: Cannot enable NtCreateFile Hook. You need to set the call number!\n" );
      }

      // save our new state
      pExtension->systemHookInfo.systemHookState = SystemHookState_Installed;
   }
}

void AppIntegrity_UnInstallSystemHook( void )
{
   DEVICE_EXTENSION *pExtension = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;

   // restore the original NtTerminateProcess
   if ( pExtension->systemHookInfo.pOriginalNtTerminateProcess )
   {
      UnHookNtSystemService( pExtension->systemHookInfo.ntTerminateProcess_CallNumber, 
                             (ULONG)pExtension->systemHookInfo.pOriginalNtTerminateProcess );

      DbgPrint( "NtTerminateProcess Hook UnInstalled!\n" );
      DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtTerminateProcess );

      // clear the system hook values so we're protected if activate is called without valid info
      pExtension->systemHookInfo.pOriginalNtTerminateProcess   = (PVOID)0;
      pExtension->systemHookInfo.ntTerminateProcess_CallNumber = 0;
   }
   else
   {
      DbgPrint( "NtTerminateProcess was never hooked, so not uninstalling hook!\n" );
   }

   // restore the original NtSetValueKey
   if ( pExtension->systemHookInfo.pOriginalNtSetValueKey )
   {
      UnHookNtSystemService( pExtension->systemHookInfo.ntSetValueKey_CallNumber, 
                             (ULONG)pExtension->systemHookInfo.pOriginalNtSetValueKey );

      DbgPrint( "NtSetValueKey Hook UnInstalled!\n" );
      DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtSetValueKey );

      pExtension->systemHookInfo.pOriginalNtSetValueKey     = (PVOID)0;
      pExtension->systemHookInfo.ntSetValueKey_CallNumber   = 0;
   }
   else
   {
      DbgPrint( "NtSetValueKey was never hooked, so not uninstalling hook!\n" );
   }

   // restore the original NtDeleteValueKey
   if ( pExtension->systemHookInfo.pOriginalNtDeleteValueKey )
   {
      UnHookNtSystemService( pExtension->systemHookInfo.ntDeleteValueKey_CallNumber, 
                             (ULONG)pExtension->systemHookInfo.pOriginalNtDeleteValueKey );

      DbgPrint( "NtDeleteValueKey Hook UnInstalled!\n" );
      DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtDeleteValueKey );

      pExtension->systemHookInfo.pOriginalNtDeleteValueKey     = (PVOID)0;
      pExtension->systemHookInfo.ntDeleteValueKey_CallNumber   = 0;
   }
   else
   {
      DbgPrint( "NtDeleteValueKey was never hooked, so not uninstalling hook!\n" );
   }

   // restore the original NtDeleteKey
   if ( pExtension->systemHookInfo.pOriginalNtDeleteKey )
   {
      UnHookNtSystemService( pExtension->systemHookInfo.ntDeleteKey_CallNumber, 
                             (ULONG)pExtension->systemHookInfo.pOriginalNtDeleteKey );

      DbgPrint( "NtDeleteKey Hook UnInstalled!\n" );
      DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtDeleteKey );

      pExtension->systemHookInfo.pOriginalNtDeleteKey     = (PVOID)0;
      pExtension->systemHookInfo.ntDeleteKey_CallNumber   = 0;
   }
   else
   {
      DbgPrint( "NtDeleteKey was never hooked, so not uninstalling hook!\n" );
   }

   // restore the original NtRenameKey
   if ( pExtension->systemHookInfo.pOriginalNtRenameKey )
   {
      UnHookNtSystemService( pExtension->systemHookInfo.ntRenameKey_CallNumber, 
                             (ULONG)pExtension->systemHookInfo.pOriginalNtRenameKey );

      DbgPrint( "NtRenameKey Hook UnInstalled!\n" );
      DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtRenameKey );

      pExtension->systemHookInfo.pOriginalNtRenameKey     = (PVOID)0;
      pExtension->systemHookInfo.ntRenameKey_CallNumber   = 0;
   }
   else
   {
      DbgPrint( "NtRenameKey was never hooked, so not uninstalling hook!\n" );
   }
   
   // restore the original NtOpenFile
   if ( pExtension->systemHookInfo.pOriginalNtOpenFile )
   {
      UnHookNtSystemService( pExtension->systemHookInfo.ntOpenFile_CallNumber, 
                             (ULONG)pExtension->systemHookInfo.pOriginalNtOpenFile );

      DbgPrint( "NtOpenFile Hook UnInstalled!\n" );
      DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtOpenFile );

      pExtension->systemHookInfo.pOriginalNtOpenFile    = (PVOID)0;
      pExtension->systemHookInfo.ntOpenFile_CallNumber   = 0;
   }
   else
   {
      DbgPrint( "NtOpenFile was never hooked, so not uninstalling hook!\n" );
   }

   // restore the original NtCreateFile
   if ( pExtension->systemHookInfo.pOriginalNtCreateFile )
   {
      UnHookNtSystemService( pExtension->systemHookInfo.ntCreateFile_CallNumber, 
                             (ULONG)pExtension->systemHookInfo.pOriginalNtCreateFile );

      DbgPrint( "NtCreateFile Hook UnInstalled!\n" );
      DbgPrint( "Info: Original Ptr: %x\n", (ULONG) pExtension->systemHookInfo.pOriginalNtCreateFile );

      pExtension->systemHookInfo.pOriginalNtCreateFile    = (PVOID)0;
      pExtension->systemHookInfo.ntCreateFile_CallNumber  = 0;
   }
   else
   {
      DbgPrint( "NtCreateFile was never hooked, so not uninstalling hook!\n" );
   }

   // save our new state
   pExtension->systemHookInfo.systemHookState = SystemHookState_UnInstalled;
}

NTSTATUS NTAPI NewNtTerminateProcess( IN HANDLE ProcessHandle OPTIONAL, IN NTSTATUS ExitStatus )
{
   DEVICE_EXTENSION           *pExtension = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   pZwQueryInformationProcess  ZwQueryInformationProcess;
   UNICODE_STRING              routineName;
   PROCESS_BASIC_INFORMATION   processInfo;
   NTSTATUS                    status;
   BOOLEAN                     allowTermination = TRUE;

   do
   {
      // first, are we even monitoring?
      if ( !(pExtension->appIntegrityItems.monitorState & MONITOR_NT_TERMINATE_PROCESS) )
      {
         break;
      }

      // get a pointer to ZwQueryInformationProcess
      RtlInitUnicodeString( &routineName, L"ZwQueryInformationProcess" );
      ZwQueryInformationProcess = MmGetSystemRoutineAddress( &routineName );

      if ( !ZwQueryInformationProcess )
      {
         DbgPrint( "Failed to get ZwQueryInformationProcess\n" );
         break;
      }
      
      status = ZwQueryInformationProcess( ProcessHandle, ProcessBasicInformation, &processInfo, sizeof( processInfo ), NULL );
      if ( status != STATUS_SUCCESS )
      {
         if ( STATUS_INVALID_HANDLE == status )
         {
            DbgPrint( "ZwQueryInformationProcess Failed: Invalid Handle\n", status );
         }
         else
         {
            DbgPrint( "ZwQueryInformationProcess Failed: %x\n", status );
         }
         break;
      }

      // check to see if the process to terminate is ours
      if ( pExtension->appIntegrityItems.processId == processInfo.UniqueProcessId )
      {
         // ok, it is. But forcefull terminations pass 0x1, so we'll only prevent that. If X9 crashed, we dont want to prevent it.
         if ( 0x1 == ExitStatus )
         {
            DbgPrint( "%d is protected Process ID %d. Denying.\n", processInfo.UniqueProcessId, pExtension->appIntegrityItems.processId );
            allowTermination = FALSE;
         }
         else
         {
            DbgPrint( "ExitStatus is not normal. We are allowing and disabling AppIntegrity. ExitStatus: 0x%x\n", ExitStatus );
            pExtension->appIntegrityItems.monitorState = 0;
         }
      }
   }
   while( 0 );
   
   if ( allowTermination )
   {
      // no problems found, allow the original terminate process to go thru
      return pExtension->systemHookInfo.pOriginalNtTerminateProcess( ProcessHandle, ExitStatus );
   }
   else
   {
      // we don't want to allow it.
      return STATUS_ACCESS_DENIED;
   }
}

NTSTATUS NTAPI NewNtSetValueKey( IN HANDLE KeyHandle, IN PUNICODE_STRING ValueName, IN ULONG TitleIndex OPTIONAL, IN ULONG Type, IN PVOID Data, IN ULONG DataSize )
{
   WCHAR                 deviceHeap[ MAX_REG_PATH_UNICODE ];
   DEVICE_EXTENSION     *pExtension   = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   KEY_NAME_INFORMATION *pNameKeyInfo = (KEY_NAME_INFORMATION *)deviceHeap; //use our device heap for the key name;
   BOOLEAN               isProtected  = FALSE;
   ULONG                 sizeNeeded   = 0;
   UNICODE_STRING        itemToDeleteKeyPath;
   ANSI_STRING           ansiString;
   USERSPACE_REG_KEY     userRegKey;
   NTSTATUS              ntStatus;

   do
   {
      // first, are we even monitoring?
      if ( !(pExtension->appIntegrityItems.monitorState & MONITOR_NT_SET_VALUE_KEY) )
      {
         break;
      }

      // ---- get the key for this value ----
      // 0 out of the memory for the key info
      memset( pNameKeyInfo, 0, sizeof( deviceHeap ) );

      // get the actual name info. This should not fail
      ntStatus = ZwQueryKey( KeyHandle, KeyNameInformation, pNameKeyInfo, sizeof( deviceHeap ), &sizeNeeded );

      if ( ntStatus != STATUS_SUCCESS )
      {
         if ( STATUS_BUFFER_OVERFLOW == ntStatus )
         {
            DbgPrint( "Need to allocate %d, but device heap is %d\n", sizeNeeded, sizeof( deviceHeap ) );
         }
         else  
         {
            DbgPrint( "ZwQueryKey Error: 0x%x\n", ntStatus );
         }
         break;
      }

      // strip out the root class
      if ( !AppIntegrity_ParseRegistryKey( pNameKeyInfo->Name, pNameKeyInfo->NameLength / sizeof( WCHAR ), &userRegKey ) ) break;

      RtlInitUnicodeString( &itemToDeleteKeyPath, userRegKey.name );

#if DBG
      // print the value that's trying to change
      RtlUnicodeStringToAnsiString( &ansiString, &itemToDeleteKeyPath, TRUE );
      DbgPrint( "Reg Set Value Attempt: %s\\", ansiString.Buffer );
      RtlFreeAnsiString( &ansiString );

      RtlUnicodeStringToAnsiString( &ansiString, ValueName, TRUE );
      DbgPrint( "%s\n", ansiString.Buffer );
      RtlFreeAnsiString( &ansiString );
#endif

      // allow alteration?
      isProtected = AppIntegrity_IsRegItemProtected( userRegKey.rootClass, &itemToDeleteKeyPath, ValueName );
   }
   while( 0 );

   if ( !isProtected )
   {
      return pExtension->systemHookInfo.pOriginalNtSetValueKey( KeyHandle, ValueName, TitleIndex, Type, Data, DataSize );
   }
   else
   {
      return STATUS_UNSUCCESSFUL;
   }
}

NTSTATUS NTAPI NewNtDeleteValueKey( IN HANDLE KeyHandle, IN PUNICODE_STRING ValueName )
{
   WCHAR                 deviceHeap[ MAX_REG_PATH_UNICODE ];
   DEVICE_EXTENSION     *pExtension   = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   KEY_NAME_INFORMATION *pNameKeyInfo = (KEY_NAME_INFORMATION *)deviceHeap; //use our device heap for the key name
   BOOLEAN               isProtected  = FALSE;
   ULONG                 sizeNeeded   = 0;
   ANSI_STRING           ansiString;
   UNICODE_STRING        itemToDeleteKeyPath;
   USERSPACE_REG_KEY     userRegKey;
   NTSTATUS              ntStatus;
   int i;

   do
   {
      // first, are we even monitoring?
      if ( !(pExtension->appIntegrityItems.monitorState & MONITOR_NT_DELETE_VALUE_KEY) )
      {
         break;
      }

      // ---- get the key for this value ----
      // 0 out of the memory for the key info
      memset( pNameKeyInfo, 0, sizeof( deviceHeap ) );

      // get the actual name info. This should not fail
      ntStatus = ZwQueryKey( KeyHandle, KeyNameInformation, pNameKeyInfo, sizeof( deviceHeap ), &sizeNeeded );

      if ( ntStatus != STATUS_SUCCESS )
      {
         if ( STATUS_BUFFER_OVERFLOW == ntStatus )
         {
            DbgPrint( "Need to allocate %d, but device heap is %d\n", sizeNeeded, sizeof( deviceHeap ) );
         }
         else  
         {
            DbgPrint( "ZwQueryKey Error: 0x%x\n", ntStatus );
         }
         break;
      }

      // strip out the root class
      if ( !AppIntegrity_ParseRegistryKey( pNameKeyInfo->Name, pNameKeyInfo->NameLength / sizeof( WCHAR ), &userRegKey ) ) break;

      RtlInitUnicodeString( &itemToDeleteKeyPath, userRegKey.name );

#if DBG
      // print the value that's trying to change
      RtlUnicodeStringToAnsiString( &ansiString, &itemToDeleteKeyPath, TRUE );
      DbgPrint( "Reg Delete Value Attempt: %s\\", ansiString.Buffer );
      RtlFreeAnsiString( &ansiString );

      RtlUnicodeStringToAnsiString( &ansiString, ValueName, TRUE );
      DbgPrint( "%s\n", ansiString.Buffer );
      RtlFreeAnsiString( &ansiString );
#endif

      // allow alteration?
      isProtected = AppIntegrity_IsRegItemProtected( userRegKey.rootClass, &itemToDeleteKeyPath, ValueName );
   }
   while( 0 );

   if ( !isProtected )
   {
      return pExtension->systemHookInfo.pOriginalNtDeleteValueKey( KeyHandle, ValueName );
   }
   else
   {
      return STATUS_UNSUCCESSFUL;
   }
}

NTSTATUS NTAPI NewNtDeleteKey( IN HANDLE KeyHandle )
{
   WCHAR                 deviceHeap[ MAX_REG_PATH_UNICODE ];
   DEVICE_EXTENSION     *pExtension    = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   KEY_NAME_INFORMATION *pNameKeyInfo  = (KEY_NAME_INFORMATION *)deviceHeap; //use our device heap for the key name
   BOOLEAN               isProtected   = FALSE;
   ULONG                 sizeNeeded    = 0;
   UNICODE_STRING        keyToDelete;
   USERSPACE_REG_KEY     userRegKey;
   ANSI_STRING           ansiString;
   NTSTATUS              ntStatus;
   int i;

   do
   {
      // first, are we even monitoring?
      if ( !(pExtension->appIntegrityItems.monitorState & MONITOR_NT_DELETE_KEY) )
      {
         break;
      }

      // 0 out of the memory for the key info
      memset( pNameKeyInfo, 0, sizeof( deviceHeap ) );

      // get the actual name info. This should not fail
      ntStatus = ZwQueryKey( KeyHandle, KeyNameInformation, pNameKeyInfo, sizeof( deviceHeap ), &sizeNeeded );

      if ( ntStatus != STATUS_SUCCESS )
      {
         if ( STATUS_BUFFER_OVERFLOW == ntStatus )
         {
            DbgPrint( "Need to allocate %d, but device heap is %d\n", sizeNeeded, sizeof( deviceHeap ) );
         }
         else  
         {
            DbgPrint( "ZwQueryKey Error: 0x%x\n", ntStatus );
         }
         break;
      }

      // strip out the root class
      if ( !AppIntegrity_ParseRegistryKey( pNameKeyInfo->Name, pNameKeyInfo->NameLength / sizeof( WCHAR ), &userRegKey ) ) break;

#if DBG
      // now put it in an ansi buffer for debugging
      RtlInitUnicodeString( &keyToDelete, pNameKeyInfo->Name );
      RtlUnicodeStringToAnsiString( &ansiString, &keyToDelete, TRUE );
      DbgPrint( "Key To Delete: %s\n", ansiString.Buffer );
      RtlFreeAnsiString( &ansiString );
#endif

      // set key to delete to our parsed key
      RtlInitUnicodeString( &keyToDelete, userRegKey.name );

      // allow alteration?
      isProtected = AppIntegrity_IsRegItemProtected( userRegKey.rootClass, &keyToDelete, NULL );
   }
   while( 0 );

   if ( !isProtected )
   {
      return pExtension->systemHookInfo.pOriginalNtDeleteKey( KeyHandle );
   }
   else
   {
      return STATUS_UNSUCCESSFUL;
   }
}

NTSTATUS NTAPI NewNtRenameKey( IN HANDLE KeyHandle, PUNICODE_STRING ReplacementName )
{
   WCHAR                 deviceHeap[ MAX_REG_PATH_UNICODE ];
   DEVICE_EXTENSION     *pExtension   = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   KEY_NAME_INFORMATION *pNameKeyInfo = (KEY_NAME_INFORMATION *)deviceHeap; //use our device heap for the key name
   BOOLEAN               isProtected  = FALSE;
   ULONG                 sizeNeeded   = 0;
   UNICODE_STRING        keyToRename;
   USERSPACE_REG_KEY     userRegKey;
   ANSI_STRING           ansiString;
   NTSTATUS              ntStatus;
   int i;

   do
   {
      // first, are we even monitoring?
      if ( !(pExtension->appIntegrityItems.monitorState & MONITOR_NT_RENAME_KEY) )
      {
         break;
      }

      // 0 out of the memory for the key info
      memset( pNameKeyInfo, 0, sizeof( deviceHeap ) );

      // get the actual name info. This should not fail
      ntStatus = ZwQueryKey( KeyHandle, KeyNameInformation, pNameKeyInfo, sizeof( deviceHeap ), &sizeNeeded );

      if ( ntStatus != STATUS_SUCCESS )
      {
         if ( STATUS_BUFFER_OVERFLOW == ntStatus )
         {
            DbgPrint( "Need to allocate %d, but device heap is %d\n", sizeNeeded, sizeof( deviceHeap ) );
         }
         else  
         {
            DbgPrint( "ZwQueryKey Error: 0x%x\n", ntStatus );
         }
         break;
      }

      // strip out the root class
      if ( !AppIntegrity_ParseRegistryKey( pNameKeyInfo->Name, pNameKeyInfo->NameLength / sizeof( WCHAR ), &userRegKey ) ) break;

#if DBG
      // now put it in an ansi buffer for debugging
      RtlInitUnicodeString( &keyToRename, pNameKeyInfo->Name );
      RtlUnicodeStringToAnsiString( &ansiString, &keyToRename, TRUE );
      DbgPrint( "Key To Rename: %s\n", ansiString.Buffer );
      RtlFreeAnsiString( &ansiString );
#endif

      // set key to rename to our parsed key
      RtlInitUnicodeString( &keyToRename, userRegKey.name );

      // allow alteration?
      isProtected = AppIntegrity_IsRegItemProtected( userRegKey.rootClass, &keyToRename, NULL );
   }
   while( 0 );

   if ( !isProtected )
   {
      return pExtension->systemHookInfo.pOriginalNtRenameKey( KeyHandle, ReplacementName );
   }
   else
   {
      return STATUS_UNSUCCESSFUL;
   }
}

NTSTATUS NTAPI NewNtOpenFile( OUT PHANDLE           FileHandle,
                              IN ACCESS_MASK        DesiredAccess,
                              IN POBJECT_ATTRIBUTES ObjectAttributes,
                              OUT PIO_STATUS_BLOCK  IoStatusBlock,
                              IN ULONG              ShareAccess,
                              IN ULONG              OpenOptions )
{
   WCHAR             deviceHeap[ MAX_REG_PATH_UNICODE ];
   DEVICE_EXTENSION *pExtension  = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   WCHAR            *pObjectPath = (WCHAR *)deviceHeap; //use our device heap for the object path
   BOOLEAN           isProtected = FALSE;
   ULONG             lengthInUnicodeChars = ObjectAttributes->ObjectName->Length / sizeof( WCHAR );
   int i;

   do
   {
      // first, are we even monitoring?
      if ( !(pExtension->appIntegrityItems.monitorState & MONITOR_NT_OPEN_FILE) )
      {
         break;
      }

      /// is the file being access only for read?
      if ( !(DesiredAccess & DELETE) && !(DesiredAccess & GENERIC_WRITE) )
      {
         break;
      }

      // is the file to create 0 bytes?
      if ( !lengthInUnicodeChars )
      {
         break;
      }

      memset( pObjectPath, 0, sizeof( deviceHeap ) );

      // copy the provided string into our heap buffer. We can clamp to windows path because we don't protect anything
      // that would be longer than that.
      RtlStringCchCopyNW( pObjectPath, MAX_WINDOWS_PATH_UNICODE - 1, ObjectAttributes->ObjectName->Buffer, min( MAX_WINDOWS_PATH_UNICODE - 1, lengthInUnicodeChars ) );

      // convert the provided string to lower case
      _wcslwr( pObjectPath );

      // test for a dir
      for ( i = 0; i < MAX_PROTECTED_DIRS; i++ )
      {
         if ( pExtension->appIntegrityItems.protectedDir[ i ].inUse )
         {
            // is a file trying to be opened in our protected folder?
            if ( wcsstr( pObjectPath, pExtension->appIntegrityItems.protectedDir[ i ].dirPath ) )
            {
               DbgPrint( "ATTEMPTING TO CALL OPEN ON FILE IN PROTECTED DIRECTORY. DENYING!\n" );
               isProtected = TRUE;
               break;
            }
         }
      }

      // test for a file
      for ( i = 0; i < MAX_PROTECTED_FILES; i++ )
      {
         if ( pExtension->appIntegrityItems.protectedFile[ i ].inUse )
         {
            // is a file trying to be opened in our protected folder?
            if ( wcsstr( pObjectPath, pExtension->appIntegrityItems.protectedFile[ i ].filePath ) )
            {
               DbgPrint( "ATTEMPTING TO CALL CREATE ON PROTECTED FILE. DENYING!\n" );
               isProtected = TRUE;
               break;
            }
         }
      }
   }
   while( 0 );

   if ( !isProtected )
   {
      return pExtension->systemHookInfo.pOriginalNtOpenFile( FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions );
   }
   else
   {
      return STATUS_ACCESS_DENIED;
   }
}

NTSTATUS NTAPI NewNtCreateFile( OUT PHANDLE           FileHandle,
                                IN ACCESS_MASK        DesiredAccess,
                                IN POBJECT_ATTRIBUTES ObjectAttributes,
                                OUT PIO_STATUS_BLOCK  IoStatusBlock,
                                IN PLARGE_INTEGER     AllocationSize OPTIONAL,
                                IN ULONG              FileAttributes,
                                IN ULONG              ShareAccess,
                                IN ULONG              CreateDisposition,
                                IN ULONG              CreateOptions,
                                IN PVOID              EaBuffer OPTIONAL,
                                IN ULONG              EaLength )
{
   WCHAR             deviceHeap[ MAX_REG_PATH_UNICODE ];
   DEVICE_EXTENSION *pExtension = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   WCHAR            *pObjectPath = (WCHAR *)deviceHeap; //use our device heap for the object path
   BOOLEAN           isProtected = FALSE;
   ULONG             lengthInUnicodeChars = ObjectAttributes->ObjectName->Length / sizeof( WCHAR );
   int i;

   do
   {
      // first, are we even monitoring?
      if ( !(pExtension->appIntegrityItems.monitorState & MONITOR_NT_CREATE_FILE) )
      {
         break;
      }

      // is the file being access only for read?
      if ( !(DesiredAccess & DELETE) && !(DesiredAccess & GENERIC_WRITE) )
      {
         break;
      }

      // is the file to create 0 bytes?
      if ( !lengthInUnicodeChars )
      {
         break;
      }

      memset( pObjectPath, 0, sizeof( deviceHeap ) );

      // copy the provided string into our heap buffer. We can clamp to windows path because we don't protect anything
      // that would be longer than that.
      RtlStringCchCopyNW( pObjectPath, MAX_WINDOWS_PATH_UNICODE - 1, ObjectAttributes->ObjectName->Buffer, min( MAX_WINDOWS_PATH_UNICODE - 1, lengthInUnicodeChars ) );

      // convert the provided string to lower case
      _wcslwr( pObjectPath );

      // test for a directory
      for ( i = 0; i < MAX_PROTECTED_DIRS; i++ )
      {
         if ( pExtension->appIntegrityItems.protectedDir[ i ].inUse )
         {
            // is a file trying to be opened in our protected folder?
            if ( wcsstr( pObjectPath, pExtension->appIntegrityItems.protectedDir[ i ].dirPath ) )
            {
               DbgPrint( "ATTEMPTING TO CALL CREATE ON FILE IN PROTECTED DIRECTORY. DENYING!\n" );
               isProtected = TRUE;
               break;
            }
         }
      }

      // test for a file
      for ( i = 0; i < MAX_PROTECTED_FILES; i++ )
      {
         if ( pExtension->appIntegrityItems.protectedFile[ i ].inUse )
         {
            // is a file trying to be opened in our protected folder?
            if ( wcsstr( pObjectPath, pExtension->appIntegrityItems.protectedFile[ i ].filePath ) )
            {
               DbgPrint( "ATTEMPTING TO CALL CREATE ON PROTECTED FILE. DENYING!\n" );
               isProtected = TRUE;
               break;
            }
         }
      }
   }
   while( 0 );

   if ( !isProtected )
   {
      return pExtension->systemHookInfo.pOriginalNtCreateFile( FileHandle, 
                                                               DesiredAccess, 
                                                               ObjectAttributes, 
                                                               IoStatusBlock, 
                                                               AllocationSize, 
                                                               FileAttributes, 
                                                               ShareAccess, 
                                                               CreateDisposition, 
                                                               CreateOptions, 
                                                               EaBuffer, 
                                                               EaLength );
   }
   else
   {
      return STATUS_ACCESS_DENIED;
   }
}

BOOLEAN AppIntegrity_IsRegItemProtected( ULONG rootClass, PUNICODE_STRING pKeyPath, PUNICODE_STRING pValueName )
{
   DEVICE_EXTENSION *pExtension     = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   BOOLEAN           itemProtected  = FALSE;
   BOOLEAN           isValue        = pValueName ? TRUE : FALSE;
   int i;

   // is this a value?
   if ( isValue )
   {
      for ( i = 0; i < MAX_PROTECTED_REG_ITEMS; i++ )
      {
         if ( pExtension->appIntegrityItems.protectedRegItem[ i ].inUse &&
              pExtension->appIntegrityItems.protectedRegItem[ i ].rootClass == rootClass )
         {
            // first, is the item in our list a value?
            if ( pExtension->appIntegrityItems.protectedRegItem[ i ].isValue )
            {
               // seperately compare the key path, then the value
               if ( !_wcsnicmp( pKeyPath->Buffer  , pExtension->appIntegrityItems.protectedRegItem[ i ].keyPath  , pKeyPath->Length   / sizeof( WCHAR ) ) &&
                    !_wcsnicmp( pValueName->Buffer, pExtension->appIntegrityItems.protectedRegItem[ i ].valueName, pValueName->Length / sizeof( WCHAR ) ) )
               {
                  //DbgPrint( "Found value to change in list of protected items.\n" );
                  itemProtected = TRUE;
                  break;
               }
            }
            // the item in our list is a key. Is this key fully protected?
            else if ( pExtension->appIntegrityItems.protectedRegItem[ i ].protectAllValues )
            {
               // then see if this key matches the value to delete's key
               if ( !_wcsnicmp( pKeyPath->Buffer, pExtension->appIntegrityItems.protectedRegItem[ i ].keyPath, pKeyPath->Length / sizeof( WCHAR ) ) )
               {
                  //DbgPrint( "Found value to change's KEY in list of protected items. All values in this key are protected.\n" );
                  itemProtected = TRUE;
                  break;
               }
            }
         }
      }
   }
   // it's a key
   else
   {
      // this is simpler. Is this key in our list of protected keys?
      for ( i = 0; i < MAX_PROTECTED_REG_ITEMS; i++ )
      {
         if ( pExtension->appIntegrityItems.protectedRegItem[ i ].inUse &&
             !pExtension->appIntegrityItems.protectedRegItem[ i ].isValue && //NOT a value means IS a key
              pExtension->appIntegrityItems.protectedRegItem[ i ].rootClass == rootClass )
         {
            // do they match? (Compare the paths and see if one is within the other)
            if ( !_wcsnicmp( pKeyPath->Buffer, pExtension->appIntegrityItems.protectedRegItem[ i ].keyPath, pKeyPath->Length / sizeof( WCHAR ) ) )
            {
               //DbgPrint( "Found key to change in list of protected items.\n" );
               itemProtected = TRUE;
               break;
            }
         }
      }
   }

   return itemProtected;
}

BOOLEAN AppIntegrity_ParseRegistryKey( WCHAR *pRegistryKey, ULONG registryKeyLen, USERSPACE_REG_KEY *pParsedRegKey )
{
   BOOLEAN success = FALSE;
   WCHAR *pCurrentPos;
   ULONG remainingKeyLen;
   UNICODE_STRING registryPrefix;
   UNICODE_STRING userPrefix;
   UNICODE_STRING machinePrefix;
   UNICODE_STRING registryKey;

   do
   {
      // create unicode strings for our prefixes
      RtlInitUnicodeString( &registryPrefix, L"\\REGISTRY\\" );
      RtlInitUnicodeString( &userPrefix    , L"USER\\" );
      RtlInitUnicodeString( &machinePrefix , L"MACHINE\\" );

      // create a unicode string for our registry key
      registryKey.Buffer        = pRegistryKey;
      registryKey.Length        = (USHORT)(registryKeyLen * sizeof( WCHAR ));
      registryKey.MaximumLength = (USHORT)(registryKeyLen * sizeof( WCHAR ));

      memset( pParsedRegKey, 0, sizeof( USERSPACE_REG_KEY ) );

      // clear the name, this reg key may only be the root.
      pParsedRegKey->name[ 0 ] = 0;

      // make sure the first word is \REGISTRY
      //if ( !_wcsnicmp( pRegistryKey, L"\\REGISTRY\\", registryKeyLen ) )
      if ( RtlPrefixUnicodeString( &registryPrefix, &registryKey, TRUE ) )
      {
         // skip the registry part (pointer is WCHAR, so divide by that size so we skip the right byte amount)
         pCurrentPos = pRegistryKey + (registryPrefix.Length / sizeof( WCHAR ));
         
         // calculate the remaining length of the string
         remainingKeyLen = registryKeyLen - ((ULONG) (pCurrentPos - pRegistryKey));
         if ( !remainingKeyLen ) break;

         // now create a unicode string of the remaining portion
         registryKey.Buffer        = pCurrentPos;
         registryKey.Length        = (USHORT)(remainingKeyLen * sizeof( WCHAR ));
         registryKey.MaximumLength = (USHORT)(remainingKeyLen * sizeof( WCHAR ));

         // is this user or machine?
         //if ( !_wcsnicmp( pCurrentPos, L"USER\\", remainingKeyLen ) )
         if ( RtlPrefixUnicodeString( &userPrefix, &registryKey, TRUE ) )
         {
            pParsedRegKey->rootClass = HKEY_CURRENT_USER;

            // skip the user part (pointer is WCHAR, so divide by that size so we skip the right byte amount)
            pCurrentPos = pCurrentPos + (userPrefix.Length / sizeof( WCHAR ));

            // calculate the remaining length of the string
            remainingKeyLen = registryKeyLen - ((ULONG) (pCurrentPos - pRegistryKey));
            if ( !remainingKeyLen ) break;

            // skip past the user id.
            pCurrentPos = wmemchr( pCurrentPos, L'\\', remainingKeyLen );
            if ( !pCurrentPos ) break;
            
            pCurrentPos++;

            // now RE-calculate the remaining length of the string
            remainingKeyLen = registryKeyLen - ((ULONG) (pCurrentPos - pRegistryKey));
            if ( !remainingKeyLen ) break;

            // now copy this into the new buffer
            RtlStringCchCopyNW( pParsedRegKey->name, (MAX_REG_PATH_UNICODE - 1) / sizeof( WCHAR ), pCurrentPos, min( remainingKeyLen, (MAX_REG_PATH_UNICODE - 1) / sizeof( WCHAR ) ) );
         }
         //else if ( !_wcsnicmp( pCurrentPos, L"MACHINE\\", remainingKeyLen ) )
         else if ( RtlPrefixUnicodeString( &machinePrefix, &registryKey, TRUE ) )
         {
            pParsedRegKey->rootClass = HKEY_LOCAL_MACHINE;

            // skip the machine part (pointer is WCHAR, so divide by that size so we skip the right byte amount)
            pCurrentPos = pCurrentPos + (machinePrefix.Length / sizeof( WCHAR ));

            // calculate the remaining length of the string
            remainingKeyLen = registryKeyLen - ((ULONG) (pCurrentPos - pRegistryKey));
            if ( !remainingKeyLen ) break;
            
            // now copy this into the new buffer. we'll take 
            RtlStringCchCopyNW( pParsedRegKey->name, (MAX_REG_PATH_UNICODE - 1) / sizeof( WCHAR ), pCurrentPos, min( remainingKeyLen, (MAX_REG_PATH_UNICODE - 1) / sizeof( WCHAR ) ) );
         }
         else
         {
            DbgPrint( "Unsupported reg key.\n" );
            break;
         }

         success = TRUE;
      }
   }
   while( 0 );

   return success;
}

NTSTATUS RegistryCallback( IN PVOID CallbackContext, OUT PVOID Argument1, OUT PVOID Argument2 )
{
   char                                deviceHeap[ MAX_REG_PATH_UNICODE ]; //this is a heap we can use for the object name info
   DEVICE_EXTENSION                    *pExtension        = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   OBJECT_NAME_INFORMATION             *pObjectNameInfo   = (OBJECT_NAME_INFORMATION *)deviceHeap;
   REG_NOTIFY_CLASS                    notifyClass        = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;
   NTSTATUS                            ntStatus           = STATUS_SUCCESS;
   REG_DELETE_KEY_INFORMATION          *pRegDeleteKeyInfo;
   REG_RENAME_KEY_INFORMATION          *pRegRenameKeyInfo;
   REG_DELETE_VALUE_KEY_INFORMATION    *pRegDeleteValueKeyInfo;
   REG_SET_VALUE_KEY_INFORMATION       *pRegSetValueKeyInfo;
   ULONG                               returnLength;
   UNICODE_STRING                      keyToModify;
   USERSPACE_REG_KEY                   userRegKey;
   ANSI_STRING                         dbgAnsiString;
   ANSI_STRING                         dbgAnsiString2;
   BOOLEAN                             isProtected = FALSE;

   switch( notifyClass )
   {
      case RegNtPreDeleteKey:
      {
         // first, are we even monitoring?
         if ( !(pExtension->appIntegrityItems.monitorState & MONITOR_NT_DELETE_KEY) )
         {
            break;
         }

         // cast to the correct object
         pRegDeleteKeyInfo = (REG_DELETE_KEY_INFORMATION *)Argument2;

         // get the name of the key they want to delete
         if ( ObQueryNameString( pRegDeleteKeyInfo->Object, pObjectNameInfo, MAX_REG_PATH_UNICODE, &returnLength ) != STATUS_SUCCESS ) break;
         if ( !pObjectNameInfo->Name.Buffer ) break;

         // strip out the root class
         if ( !AppIntegrity_ParseRegistryKey( (WCHAR *)pObjectNameInfo->Name.Buffer, pObjectNameInfo->Name.Length / sizeof( WCHAR ), &userRegKey ) ) break;

         // set key to delete to our parsed key
         RtlInitUnicodeString( &keyToModify, userRegKey.name );

         // allow alteration?
         isProtected = AppIntegrity_IsRegItemProtected( userRegKey.rootClass, &keyToModify, NULL );
         
         if ( isProtected )
         {
#if DBG
            // now put it in an ansi buffer for debugging
            RtlInitUnicodeString( &keyToModify,  (WCHAR *)userRegKey.name );
            RtlUnicodeStringToAnsiString( &dbgAnsiString, &keyToModify, TRUE );
            DbgPrint( "Key To Delete: '%s'\n", dbgAnsiString.Buffer );
            RtlFreeAnsiString( &dbgAnsiString );
#endif
            // disallow this protected key from being deleted
            ntStatus = STATUS_ACCESS_DENIED;
         }

         break;
      }

      case RegNtPreRenameKey:
      {
         // first, are we even monitoring?
         if ( !(pExtension->appIntegrityItems.monitorState & MONITOR_NT_RENAME_KEY) )
         {
            break;
         }

         // cast to the correct object
         pRegRenameKeyInfo = (REG_RENAME_KEY_INFORMATION *)Argument2;

         // get the name of the key they want to delete
         if ( ObQueryNameString( pRegRenameKeyInfo->Object, pObjectNameInfo, MAX_REG_PATH_UNICODE, &returnLength ) != STATUS_SUCCESS ) break;
         if ( !pObjectNameInfo->Name.Buffer ) break;

         // strip out the root class
         if ( !AppIntegrity_ParseRegistryKey( (WCHAR *)pObjectNameInfo->Name.Buffer, pObjectNameInfo->Name.Length / sizeof( WCHAR ), &userRegKey ) ) break;

         // set key to delete to our parsed key
         RtlInitUnicodeString( &keyToModify, userRegKey.name );

         // allow alteration?
         isProtected = AppIntegrity_IsRegItemProtected( userRegKey.rootClass, &keyToModify, NULL );
         
         if ( isProtected )
         {
#if DBG
            // now put it in an ansi buffer for debugging
            RtlInitUnicodeString( &keyToModify,  (WCHAR *)userRegKey.name );
            RtlUnicodeStringToAnsiString( &dbgAnsiString, &keyToModify, TRUE );
            DbgPrint( "Key To Rename: '%s'\n", dbgAnsiString.Buffer );
            RtlFreeAnsiString( &dbgAnsiString );
#endif
            // disallow this protected key from being deleted
            ntStatus = STATUS_ACCESS_DENIED;
         }
      }

      case RegNtPreDeleteValueKey:
      {
         // first, are we even monitoring?
         if ( !(pExtension->appIntegrityItems.monitorState & MONITOR_NT_DELETE_VALUE_KEY) )
         {
            break;
         }

         // cast to the correct object
         pRegDeleteValueKeyInfo = (REG_DELETE_VALUE_KEY_INFORMATION *)Argument2;

         // get the name of the key of the value they want to delete
         if ( ObQueryNameString( pRegDeleteValueKeyInfo->Object, pObjectNameInfo, MAX_REG_PATH_UNICODE, &returnLength ) != STATUS_SUCCESS ) break;
         if ( !pObjectNameInfo->Name.Buffer ) break;

         // strip out the root class
         if ( !AppIntegrity_ParseRegistryKey( (WCHAR *)pObjectNameInfo->Name.Buffer, pObjectNameInfo->Name.Length / sizeof( WCHAR ), &userRegKey ) ) break;

         // set the value to delete to our parsed key
         RtlInitUnicodeString( &keyToModify, userRegKey.name );

         // allow alteration?
         isProtected = AppIntegrity_IsRegItemProtected( userRegKey.rootClass, &keyToModify, pRegDeleteValueKeyInfo->ValueName );

         if ( isProtected )
         {
#if DBG
            // print the value that's trying to change
            RtlInitUnicodeString( &keyToModify,  (WCHAR *)userRegKey.name );
            RtlUnicodeStringToAnsiString( &dbgAnsiString, &keyToModify, TRUE );
            RtlUnicodeStringToAnsiString( &dbgAnsiString2, pRegDeleteValueKeyInfo->ValueName, TRUE );

            DbgPrint( "Reg Delete Value Attempt: Key: '%s' Value: '%s'", dbgAnsiString.Buffer, dbgAnsiString2.Buffer );
            RtlFreeAnsiString( &dbgAnsiString );
            RtlFreeAnsiString( &dbgAnsiString2 );
#endif
            // disallow this protected key from being deleted
            ntStatus = STATUS_ACCESS_DENIED;
         }

         break;
      }

      case RegNtPreSetValueKey:
      {
         // first, are we even monitoring?
         if ( !(pExtension->appIntegrityItems.monitorState & MONITOR_NT_SET_VALUE_KEY) )
         {
            break;
         }

         // cast to the correct object
         pRegSetValueKeyInfo = (REG_SET_VALUE_KEY_INFORMATION *)Argument2;

         // get the name of the key of the value they want to delete
         if ( ObQueryNameString( pRegSetValueKeyInfo->Object, pObjectNameInfo, MAX_REG_PATH_UNICODE, &returnLength ) != STATUS_SUCCESS ) break;
         if ( !pObjectNameInfo->Name.Buffer ) break;

         // strip out the root class
         if ( !AppIntegrity_ParseRegistryKey( (WCHAR *)pObjectNameInfo->Name.Buffer, pObjectNameInfo->Name.Length / sizeof( WCHAR ), &userRegKey ) ) break;

         // set value to set to our parsed key
         RtlInitUnicodeString( &keyToModify, userRegKey.name );

         // allow alteration?
         isProtected = AppIntegrity_IsRegItemProtected( userRegKey.rootClass, &keyToModify, pRegSetValueKeyInfo->ValueName );

         if ( isProtected )
         {
#if DBG
            // print the value that's trying to change
            RtlInitUnicodeString( &keyToModify,  (WCHAR *)userRegKey.name );
            RtlUnicodeStringToAnsiString( &dbgAnsiString, &keyToModify, TRUE );
            RtlUnicodeStringToAnsiString( &dbgAnsiString2, pRegSetValueKeyInfo->ValueName, TRUE );

            DbgPrint( "Reg Set Value Attempt: Key: '%s' Value: '%s'", dbgAnsiString.Buffer, dbgAnsiString2.Buffer );
            RtlFreeAnsiString( &dbgAnsiString );
            RtlFreeAnsiString( &dbgAnsiString2 );
#endif
            // disallow this protected key from being deleted
            ntStatus = STATUS_ACCESS_DENIED;
         }

         break;
      }
   }

   return ntStatus;
}

VOID CreateProcessNotifyEx( __inout PEPROCESS               Process,
                            __in HANDLE                     ProcessId,
                            __in_opt PPS_CREATE_NOTIFY_INFO CreateInfo )
{
   DEVICE_EXTENSION *pExtension = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;

   // if our monitoring state is on, we do not want to accept a new object. It's simply
   // another instance of something we're already protecting.
   if ( (pExtension->appIntegrityItems.monitorState & MONITOR_NT_TERMINATE_PROCESS) )
   {
      return;
   }

   if ( CreateInfo )
   {
      if ( CreateInfo->CommandLine )
      {
         // see if it matches our name. If it does, take the process handle.
         if ( wcsstr( CreateInfo->CommandLine->Buffer, PROTECTED_PROCESS_NAME ) )
         {
            pExtension->appIntegrityItems.processObject = Process;

            DbgPrint( "Protected process %s found.\n", PROTECTED_PROCESS_NAME );
         }
      }
   }
}

OB_PREOP_CALLBACK_STATUS ObjectPreCallback( IN PVOID RegistrationContext, IN POB_PRE_OPERATION_INFORMATION OperationInformation )
{
   DEVICE_EXTENSION                 *pExtension = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   OB_PRE_CREATE_HANDLE_INFORMATION *pObPreCreateHandleInfo;

   switch( OperationInformation->Operation )
   {
      case OB_OPERATION_HANDLE_CREATE:
      {
         // make sure the object being created _is_ a process (as opposed to a thread)
         if ( *PsProcessType == OperationInformation->ObjectType )
         {
            // make sure this isn't us invoking our own callback
            if ( OperationInformation->Object != PsGetCurrentProcess( ) )
            {
               // we know it's a process, so cast the params accordingly
               pObPreCreateHandleInfo = (OB_PRE_CREATE_HANDLE_INFORMATION *)OperationInformation->Parameters;

               // does this object matches our process to protect?
               if ( pExtension->appIntegrityItems.processObject == OperationInformation->Object )
               {
                  // are we monitoring?
                  if ( (pExtension->appIntegrityItems.monitorState & MONITOR_NT_TERMINATE_PROCESS) )
                  {
                     // clear out the terminate process flag; we don't want to allow it to be shut down.
                     pObPreCreateHandleInfo->DesiredAccess &= ~PROCESS_TERMINATE;
                  }
                  else
                  {
                     // if not, put BACK the termination flag.
                     pObPreCreateHandleInfo->DesiredAccess |= PROCESS_TERMINATE;
                  }
               }
            }
         }
               
         break;
      }
   }

   return OB_PREOP_SUCCESS;
}

NTSTATUS FilterManagerUnload( __in FLT_FILTER_UNLOAD_FLAGS Flags )
{
   DEVICE_EXTENSION *pExtension = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;

   FltUnregisterFilter( pExtension->systemCallbackInfo.filterHandle );

   return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS PtPreOperationPassThrough( __inout PFLT_CALLBACK_DATA Data,
                                                     __in PCFLT_RELATED_OBJECTS FltObjects,
                                                     __deref_out_opt PVOID *CompletionContext )
{
   WCHAR                       deviceHeap[ MAX_REG_PATH_UNICODE ];
   DEVICE_EXTENSION           *pExtension = (DEVICE_EXTENSION *)g_pDeviceObject->DeviceExtension;
   WCHAR                      *pObjectPath = deviceHeap; //use our device heap for the object path
   PFLT_FILE_NAME_INFORMATION  fileNameInfo;
   BOOLEAN                     isProtected = FALSE;
   ULONG                       lengthInUnicodeChars;
   int i;

   do
   {
      // first, are we even monitoring?
      if ( !(pExtension->appIntegrityItems.monitorState & MONITOR_NT_CREATE_FILE) )
      {
         break;
      }


      // if there's no file object, there's nothing for us to do.
      if ( !FltObjects->FileObject )
      {
         break;
      }

      // if we fail to get the info for this file, forget it
      if ( FltGetFileNameInformation( Data, FLT_FILE_NAME_NORMALIZED, &fileNameInfo ) != STATUS_SUCCESS )
      {
         break;
      }

      // is the file to create 0 bytes?
      lengthInUnicodeChars = fileNameInfo->Name.Length / sizeof( WCHAR );
      if ( !lengthInUnicodeChars )
      {
         break;
      }

      memset( pObjectPath, 0, sizeof( deviceHeap ) );

      // copy the provided string into our heap buffer. We can clamp to windows path because we don't protect anything
      // that would be longer than that.
      RtlStringCchCopyNW( pObjectPath, MAX_WINDOWS_PATH_UNICODE - 1, fileNameInfo->Name.Buffer, min( MAX_WINDOWS_PATH_UNICODE - 1, lengthInUnicodeChars ) );

      // free the file name info, now that we've copied what we need
      FltReleaseFileNameInformation( fileNameInfo );

      // convert the provided string to lower case. This is safely null terminated, because we
      // zero out the memory in the memset above.
      _wcslwr( pObjectPath );

      // test for a directory
      for ( i = 0; i < MAX_PROTECTED_DIRS; i++ )
      {
         if ( pExtension->appIntegrityItems.protectedDir[ i ].inUse )
         {
            // is a file trying to be opened in our protected folder?
            if ( wcsstr( pObjectPath, pExtension->appIntegrityItems.protectedDir[ i ].dirPath ) )
            {
               if ( FltObjects->FileObject->WriteAccess || FltObjects->FileObject->DeleteAccess || FltObjects->FileObject->DeletePending /*|| FltObjects->FileObject->SharedDelete*/ )
               {
                  DbgPrint( "ATTEMPTING TO CALL CREATE ON FILE IN PROTECTED DIRECTORY. DENYING!\n" );
                  isProtected = TRUE;
                  break;
               }
            }
         }
      }

      // test for a file
      for ( i = 0; i < MAX_PROTECTED_FILES; i++ )
      {
         if ( pExtension->appIntegrityItems.protectedFile[ i ].inUse )
         {
            // is a file trying to be opened in our protected folder?
            if ( wcsstr( pObjectPath, pExtension->appIntegrityItems.protectedFile[ i ].filePath ) )
            {
               if ( FltObjects->FileObject->WriteAccess || FltObjects->FileObject->DeleteAccess || FltObjects->FileObject->DeletePending /*|| FltObjects->FileObject->SharedDelete*/ )
               {
                  DbgPrint( "ATTEMPTING TO CALL CREATE ON PROTECTED FILE. DENYING!\n" );
                  isProtected = TRUE;
                  break;
               }
            }
         }
      }
   }
   while( 0 );

   if ( !isProtected )
   {
      // if not protected, allow the rest of the filters to process
      return FLT_PREOP_SUCCESS_NO_CALLBACK;
   }
   else
   {
      // flag that we do not allow this type of modification (writing/deleting)
      Data->IoStatus.Status = STATUS_ACCESS_DENIED;
      
      return FLT_PREOP_COMPLETE;
   }
}
