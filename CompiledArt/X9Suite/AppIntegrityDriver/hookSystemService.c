
#include "hookSystemService.h"
#include <ntddk.h>

// system hooking doesn't work on 64bit architectures
#ifdef _AMD64_
ULONG HookNtSystemService( ULONG serviceCallIndex, ULONG newServiceCallAddress )
{
   return 0;
}

void UnHookNtSystemService( ULONG serviceCallIndex, ULONG oldServiceCallAddress )
{
   HookNtSystemService( serviceCallIndex, oldServiceCallAddress );
}
#elif defined _X86_

// define the exported Service Table
typedef struct ServiceDescriptorEntry 
{
   unsigned int *ServiceTableBase;
   unsigned int *ServiceCounterTableBase; //Used only in checked build
   unsigned int NumberOfServices;
   unsigned char *ParamTableBase;
}
ServiceDescriptorTableEntry, *PServiceDescriptorTableEntry;

extern PServiceDescriptorTableEntry KeServiceDescriptorTable;

ULONG HookNtSystemService( ULONG serviceCallIndex, ULONG newServiceCallAddress )
{
    //store original function
    ULONG originalServiceCallAddress = KeServiceDescriptorTable->ServiceTableBase[ serviceCallIndex ]; 

    _asm 
    {
       CLI                  //dissable interrupt
       MOV EAX, CR0         //move CR0 register into EAX
       AND EAX, NOT 10000H //disable WP bit
       MOV CR0, EAX        //write register back
    }

    // hook the requested function
    KeServiceDescriptorTable->ServiceTableBase[ serviceCallIndex ] = (ULONG)newServiceCallAddress;

    _asm
    {
       MOV EAX, CR0   //move CR0 register into EAX
       OR EAX, 10000H //enable WP bit
       MOV CR0, EAX   //write register back
       STI            //enable interrupt
    }

    return originalServiceCallAddress;
}

void UnHookNtSystemService( ULONG serviceCallIndex, ULONG oldServiceCallAddress )
{
   HookNtSystemService( serviceCallIndex, oldServiceCallAddress );
}

ULONG GetCurrentSystemServiceAddress( ULONG serviceCallIndex )
{
   return KeServiceDescriptorTable->ServiceTableBase[ serviceCallIndex ]; 
}

ULONG GetKeServiceDescriptorTableAddress( void )
{
   return(ULONG)KeServiceDescriptorTable;
}

#endif
