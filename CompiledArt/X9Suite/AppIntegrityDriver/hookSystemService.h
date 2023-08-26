
#ifndef HOOK_SYSTEM_SERVICE_H_
#define HOOK_SYSTEM_SERVICE_H_

typedef unsigned long ULONG;

ULONG HookNtSystemService( ULONG serviceCallIndex, ULONG newServiceCallAddress );
void UnHookNtSystemService( ULONG serviceCallIndex, ULONG oldServiceCallAddress );

#endif
