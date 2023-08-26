
#include "hookAPI.h"

#define PE_SIGNATURE (0x00004550)

int HookAPIFunction( HMODULE hModule, const char *pCurrentFunctionAddress, void *pCurrentFunction, void *pNewFunction )
{
   int modulesHooked = 0;

   do
   {
      // now we want to find the import address table of the module passed in. 
      DWORD baseAddress = (DWORD) hModule;

      // first access the DOS header which can tell us where the NT header is
      IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *) baseAddress;

      IMAGE_NT_HEADERS *pNTHeader = (IMAGE_NT_HEADERS *) (baseAddress + pDosHeader->e_lfanew);

      // now verify that it's a proper NT process
      if ( pNTHeader->Signature != PE_SIGNATURE )
      {
         OutputDebugString( "Bad NT Signature" );
         break;
      }

      // Next we want to go to the Import Address Table. The location should be in the directory listing of the optional header.
      IMAGE_IMPORT_DESCRIPTOR *pImportDesc = (IMAGE_IMPORT_DESCRIPTOR *) (baseAddress + pNTHeader->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress);

      // the Import Address Table, at a high level, consits of:
      // 1 IMAGE_IMPORT_DESCRIPTOR per DLL used by the process
      // 2 Each IMAGE_IMPORT_DESCRIPTOR contains an IMAGE_THUNK_DATA for each function imported from that DLL
      // 3 We reach NULL when there are no more THUNKS
      // 4 We reach NULL when there are no more IMAGE_IMPORT_DESCRIPTORS

      // let's find the function we're interested in
      BOOL foundFunction = FALSE;
      while( pImportDesc->Name )
      {
         // we found the DLL, now find the function.
         int thunksRead = 0;
         IMAGE_THUNK_DATA *pThunkData = (IMAGE_THUNK_DATA *) (baseAddress + pImportDesc->FirstThunk);

         while( pThunkData->u1.Function )
         {
            if ( (DWORD)pCurrentFunction == pThunkData->u1.Function )
            {
               // we found the function by address! Easy, we're done.
               OutputDebugString( "FOUND BY ADDRESS" );
               foundFunction = TRUE;
            }
            else
            {
               // see if it matches by name, if it does, it's still the process we need.
               if ( pImportDesc->OriginalFirstThunk )
               {
                  // let's see if we can find it by name under the image lookup table.
                  IMAGE_THUNK_DATA *pNameThunk = (IMAGE_THUNK_DATA *) (baseAddress + pImportDesc->OriginalFirstThunk);

                  // now offset it to the matching thunk we're on, since the lookup table is 1:1 with the IAT. (The lookup is the unbound IAT)
                  pNameThunk += thunksRead;
                  
                  // make sure there's a name exported
                  if ( (pNameThunk->u1.AddressOfData & 0x80000000) != 0x80000000 )
                  {
                     // now get a pointer to the name table of this imported function
                     IMAGE_IMPORT_BY_NAME *pRealNameThunk = (IMAGE_IMPORT_BY_NAME *) (baseAddress + pNameThunk->u1.Function);

                     // and finally check its name against the name we want
                     if ( !strcmp( pCurrentFunctionAddress, (const char *) (pRealNameThunk->Name) ) )
                     {
                        // we found it! this is our function so take it as we normally would
                        OutputDebugString( "FOUND BY NAME" );
                        foundFunction = TRUE;
                     }
                  }
               }
            }

            // SWAP FUNCTIONS
            if ( foundFunction )
            {
               // we found the function to hook.

               // this section of memory is not writeable by default, so let's change that.
               DWORD oldProtect;
               BOOL result = VirtualProtect( &pThunkData->u1.Function, sizeof( DWORD ), PAGE_EXECUTE_READWRITE, &oldProtect );
               if ( result )
               {
                  OutputDebugString( "Change Write Access OK" );
               }

               // replace the address
               pThunkData->u1.Function = (DWORD)pNewFunction;

               // reset the access rights to this memory
               DWORD newProtect = oldProtect;
               VirtualProtect( &pThunkData->u1.Function, sizeof( DWORD ), newProtect, &oldProtect );

               modulesHooked++;

               foundFunction = FALSE;
            }

            pThunkData++;
            thunksRead++;
         }

         pImportDesc++;
      }
   }
   while( 0 );

   return modulesHooked;
}
