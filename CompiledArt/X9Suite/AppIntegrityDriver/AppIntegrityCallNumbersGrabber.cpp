#include <windows.h>
#include <winternl.h>

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


void main( void )
{
	//TO USE-
	// comment _IN_ the NtFunction whose call number you want.
	// put a breakpoint at the line it gets called and jump into dissasembly.
	// Step in using F11 till you get to SYSENTER, and put a breakpoint there.
	// Now look at the value in the EAX register; that's the function's call number.
	// For the rest of the functions you need, you can simply put a breakpoint at the function,
	// then choose Debug->Enable All Breakpoints and hit F5, and it will break at SYSENTER again.
	// By default, all dissasembly breakpoints get turned off after each debugging session.

	// !NOTE! The address of NTDLL's SYSENTER will change each time you reboot, so you'll need to
	// look it up again!

	// get NTDLL, which contains all the NtFunctions
	HMODULE hModule = LoadLibrary( L"NTDLL.dll" );

	// now get the address of each function and call it
	pNtTerminateProcess ntTerminateProcess = (pNtTerminateProcess) GetProcAddress( hModule, "NtTerminateProcess" );
	//ntTerminateProcess( 0, 0 );

	pNtSetValueKey ntSetValueKey = (pNtSetValueKey) GetProcAddress( hModule, "NtSetValueKey" );
	//ntSetValueKey( 0, 0, 0, 0, 0, 0 );

	pNtDeleteValueKey ntDeleteValueKey = (pNtDeleteValueKey) GetProcAddress( hModule, "NtDeleteValueKey" );
	//ntDeleteValueKey( 0, 0 );

	pNtDeleteKey ntDeleteKey = (pNtDeleteKey) GetProcAddress( hModule, "NtDeleteKey" );
	//ntDeleteKey( 0 );

	pNtRenameKey ntRenameKey = (pNtRenameKey) GetProcAddress( hModule, "NtRenameKey" );
	//ntRenameKey( 0, 0 );

	pNtOpenFile ntOpenFile = (pNtOpenFile) GetProcAddress( hModule, "NtOpenFile" );
	//ntOpenFile( 0, 0, 0, 0, 0, 0 );

	pNtCreateFile ntCreateFile = (pNtCreateFile) GetProcAddress( hModule, "NtCreateFile" );
	ntCreateFile( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );

	
	return;
}