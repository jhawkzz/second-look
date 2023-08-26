
#ifndef SECURITY_ATTRIBS_H_
#define SECURITY_ATTRIBS_H_

class SecurityAttribs
{
   public:
      static BOOL CreateSecurityDescriptor( SECURITY_DESCRIPTOR *pSecurityDescriptor, ACL **ppAcl, LPSTR userGroup, DWORD accessPermissions );
};

#endif
