
#include "precompiled.h"
#include "securityAttribs.h"

BOOL SecurityAttribs::CreateSecurityDescriptor( SECURITY_DESCRIPTOR *pSecurityDescriptor, ACL **ppAcl, LPSTR userGroup, DWORD accessPermissions )
{
   BOOL success = FALSE;

   do
   {
      // prepare a security descriptor that lets anyone read/write
      if ( !InitializeSecurityDescriptor( pSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION ) ) break;

      // create an ACE that gives the group passed in the permissions passed in
      EXPLICIT_ACCESS explicitAccess;
      BuildExplicitAccessWithName( &explicitAccess, userGroup, accessPermissions, SET_ACCESS, NO_INHERITANCE );

      // create a dacl
      if ( SetEntriesInAcl( 1, &explicitAccess, NULL, ppAcl ) != ERROR_SUCCESS ) break;

      // re-init the security descriptor
      if ( !InitializeSecurityDescriptor( pSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION ) ) break;

      // set the dacl in this descriptor
      if ( !SetSecurityDescriptorDacl( pSecurityDescriptor, TRUE, *ppAcl, FALSE ) ) break;

      success = TRUE;
   }
   while( 0 );

   // note that we do NOT delete the ACL. That's up to the caller to do, because 
   // security descriptors reference, not copy, the ACL provided.

   return success;
}
