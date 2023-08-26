
#ifndef APP_INTEGRITY_DRIVER_DEFINES_H_
#define APP_INTEGRITY_DRIVER_DEFINES_H_

// The protected process name _must_ be compiled into the driver
#define PROTECTED_PROCESS_NAME L"Second Look"

// ---- Driver Defines ----
#define DRIVER_NAME_NON_UNICODE  "AppIntegrityDriver"
#define DRIVER_NAME             L"AppIntegrityDriver"
#define DRIVER_DESC             L"Ensures application integrity."

// ---- IO Controls ----

// these are for system hooking
#define IOCTL_APPINTEGRITY_ACTIVATE_MONITORING       CTL_CODE( FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )
#define IOCTL_APPINTEGRITY_DEACTIVATE_MONITORING     CTL_CODE( FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )
#define IOCTL_APPINTEGRITY_SET_PROCESS_ID            CTL_CODE( FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )
#define IOCTL_APPINTEGRITY_SET_PROTECTED_REG_ITEM    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )
#define IOCTL_APPINTEGRITY_CLEAR_PROTECTED_REG_LIST  CTL_CODE( FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )
#define IOCTL_APPINTEGRITY_SET_PROTECTED_DIR         CTL_CODE( FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )
#define IOCTL_APPINTEGRITY_CLEAR_PROTECTED_DIR_LIST  CTL_CODE( FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )
#define IOCTL_APPINTEGRITY_SET_PROTECTED_FILE        CTL_CODE( FILE_DEVICE_UNKNOWN, 0x807, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )
#define IOCTL_APPINTEGRITY_CLEAR_PROTECTED_FILE_LIST CTL_CODE( FILE_DEVICE_UNKNOWN, 0x808, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )

// these defines whether the various hook functions are active or not
#define MONITOR_NT_TERMINATE_PROCESS (0x1)
#define MONITOR_NT_DELETE_VALUE_KEY  (0x1 << 1)
#define MONITOR_NT_SET_VALUE_KEY     (0x1 << 2)
#define MONITOR_NT_DELETE_KEY        (0x1 << 3)
#define MONITOR_NT_RENAME_KEY        (0x1 << 4)
#define MONITOR_NT_OPEN_FILE         (0x1 << 5)
#define MONITOR_NT_CREATE_FILE       (0x1 << 6)
#define MONITOR_ALL_NT_SYSTEM_HOOKS  (MONITOR_NT_TERMINATE_PROCESS |\
                                      MONITOR_NT_SET_VALUE_KEY     |\
                                      MONITOR_NT_DELETE_VALUE_KEY  |\
                                      MONITOR_NT_DELETE_KEY        |\
                                      MONITOR_NT_RENAME_KEY        |\
                                      MONITOR_NT_OPEN_FILE         |\
                                      MONITOR_NT_CREATE_FILE)

#define MAX_WINDOWS_PATH_UNICODE (260) //This is in unicode chars
#define MAX_REG_PATH_UNICODE     (300) //This is in unicode chars

// a struct for usermode, this lets them tell us what reg items to protect
typedef struct _PROTECTED_REG_ITEM_INFO
{
   BOOLEAN remove;                    //true if we should remove protection
   BOOLEAN isValue;                   //false if key, true if value
   BOOLEAN protectAllValues;          //if a key and true, this will protect all values in key
   ULONG   rootClass;                 //hlm, hcu, etc
   WCHAR   keyPath[ MAX_REG_PATH_UNICODE ];   //the key path
   WCHAR   valueName[ MAX_REG_PATH_UNICODE ]; //the value name (if applicable)
}
PROTECTED_REG_ITEM_INFO;

// a struct for usermode, this lets them tell us what directory to protect
typedef struct _PROTECTED_DIR_INFO
{
   BOOLEAN remove;
   WCHAR dirPath[ MAX_WINDOWS_PATH_UNICODE ];
}
PROTECTED_DIR_INFO;

// a struct for usermode, this lets them tell us what file to protect
typedef struct _PROTECTED_FILE_INFO
{
   BOOLEAN remove;
   WCHAR filePath[ MAX_WINDOWS_PATH_UNICODE ];
}
PROTECTED_FILE_INFO;

#endif
