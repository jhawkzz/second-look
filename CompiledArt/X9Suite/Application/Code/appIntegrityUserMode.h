
#ifndef APP_INTEGRITY_USER_MODE_H_
#define APP_INTEGRITY_USER_MODE_H_

class AppIntegrityUserMode
{
   public:

      
                       AppIntegrityUserMode  ( void );
                       ~AppIntegrityUserMode ( );

   static DWORD WINAPI AppIntegrityThreadProc( LPVOID lpParameter );

          BOOL         EnableIntegrity       ( void );
          BOOL         DisableIntegrity      ( void );

          // This will allow us to toggle protecting the options registry key so that
          // we can update user preferences when necessary.
          BOOL         EnableOptionsRegKey   ( void );
          BOOL         DisableOptionsRegKey  ( void );
          
   private:
          HANDLE        m_hDriver;

};

#endif
