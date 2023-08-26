
#include "precompiled.h"
#include "securityAttribs.h"
#include "options.h"
#include "encrypt.h"
#include "main.h"

const char  OptionsControl::CompanyDomain[]= "http://"COMPANY_HOME;
const char  OptionsControl::RegisterPage[] = "http://"COMPANY_HOME"?product="APP_NAME"&key=register";
const char  OptionsControl::SupportPage[]  = "http://"COMPANY_HOME"?product="APP_NAME"&key=support";
const char  OptionsControl::AppName[]      = APP_NAME;
const char  OptionsControl::CompanyName[]  = COMPANY_NAME;
const char  OptionsControl::OptionsRegKey[]    = "Software\\"COMPANY_NAME"\\"APP_NAME;
const char  OptionsControl::RegistryRunValue[] = APP_NAME;
const char  OptionsControl::RunOnceMutex[] = APP_NAME" is already running.";

const float OptionsControl::sScreenCapSize[ ScreenCapSize_Total ] =
{
   0,
   640.0f,
   320.0f
};

OptionsControl::OptionsControl( void )
{
   m_IsDirty             = FALSE;
   m_hSettingsFileHandle = INVALID_HANDLE_VALUE;
}

OptionsControl::~OptionsControl( )
{
   free( m_Options.pPrivateKey );
   free( m_Options.pCertificate );

   if ( m_hSettingsFileHandle != INVALID_HANDLE_VALUE )
   {
      CloseHandle( m_hSettingsFileHandle );
   }
}

void OptionsControl::Create( void )
{
   // set all our defaults
   SetDefaultOptions( );

   // now try to load existing options
   LoadOptions( );

   ACL *pDacl = NULL;
   SECURITY_DESCRIPTOR securityDescriptor;
   SecurityAttribs::CreateSecurityDescriptor( &securityDescriptor, &pDacl, TEXT("EVERYONE"), GENERIC_READ | GENERIC_WRITE );

   SECURITY_ATTRIBUTES securityAttributes;
   securityAttributes.nLength              = sizeof( securityAttributes );
   securityAttributes.bInheritHandle       = FALSE;
   securityAttributes.lpSecurityDescriptor = &securityDescriptor;

   m_hSettingsFileHandle = CreateFile( m_Options.settingsFile, GENERIC_READ, 0, &securityAttributes, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

   LocalFree( pDacl );
}

void OptionsControl::SetDefaultOptions( void )
{
   free( m_Options.pPrivateKey );
   free( m_Options.pCertificate );

   memset( &m_Options, 0, sizeof(m_Options) );

   m_Options.port = OptionsControl::DefaultWebPort;

   char appDataDir[ MAX_PATH ];
   SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataDir );

   _snprintf( m_Options.databasePath, sizeof(m_Options.databasePath) - 1, "%s\\%s\\", appDataDir, OptionsControl::AppName );

   _snprintf( m_Options.databaseName, sizeof(m_Options.databaseName) - 1, "%s.pak", OptionsControl::AppName );

   _snprintf( m_Options.settingsFile, sizeof(m_Options.settingsFile) - 1, "%s"SETTINGS_NAME, m_Options.databasePath );

   strncpy( m_Options.login, "Admin", sizeof(m_Options.login) - 1 );
   strncpy( m_Options.password, "password", sizeof(m_Options.password) - 1 );
   
   m_Options.productKey.key[ 0 ] = NULL;

   m_Options.pPrivateKey  = NULL;
   m_Options.pCertificate = NULL;

   m_Options.privateKeySize  = 0;
   m_Options.certificateSize = 0;
   
   m_Options.daysToKeep = 7;
   m_Options.picsPerHour = 60;

   g_Options.m_Options.email.picsPerHour = 1;

   m_Options.showInSystray = 1;
   m_Options.showThumbnails = 1;
   m_Options.allowRemoteAccess = 0;
   m_Options.screenCapSize = ScreenCapSize_Full;

   // store the dimensions of the screen
   m_Options.xScreenSize = GetSystemMetrics( SM_CXVIRTUALSCREEN );
   m_Options.yScreenSize = GetSystemMetrics( SM_CYVIRTUALSCREEN );

   memset( m_Options.days, 1, sizeof(m_Options.days) );

   {
      //check for our hidden file which says the time we were installed

      char path[ MAX_PATH ] = { 0 };

      _snprintf( path, sizeof(path) - 1, "%s\\"TIMESTAMP_NAME, appDataDir );

      FILE *pFile = fopen( path, "rb" );
      
      if ( NULL != pFile )
      {
         //if the file exists, read the time it was installed
         //so we can see if it's expired
         fread( &m_Options.installTime, sizeof(m_Options.installTime), 1, pFile );
         fclose( pFile );
      }
      else
      {
         //if the file doesn't exist then it's never been installed
         //and run, so get the current time and save this file
         GetSystemTimeAsFileTime( &m_Options.installTime );

         pFile = fopen( path, "wb" );

         if ( NULL != pFile )
         {
            fwrite( &m_Options.installTime, sizeof(m_Options.installTime), 1, pFile );
            fclose( pFile );
         }
      }
   }

}

BOOL OptionsControl::LoadOptions( void )
{
   // write the options struct
   Encryptor encrypt;

   uint32 version;

   if ( FALSE == ReadValue(m_Options.settingsFile, &encrypt, "Version", &version, sizeof(version)) )
   {
      return FALSE;
   }

   ReadValue( m_Options.settingsFile, &encrypt, "privateKeySize", &m_Options.privateKeySize, sizeof(m_Options.privateKeySize) );
   ReadValue( m_Options.settingsFile, &encrypt, "certificateSize", &m_Options.certificateSize, sizeof(m_Options.certificateSize) );
   ReadValue( m_Options.settingsFile, &encrypt, "keyExpireTime", &m_Options.keyExpireTime, sizeof(m_Options.keyExpireTime) );
   ReadValue( m_Options.settingsFile, &encrypt, "days", m_Options.days, sizeof(m_Options.days) );
   ReadValue( m_Options.settingsFile, &encrypt, "port", &m_Options.port, sizeof(m_Options.port) );
   ReadValue( m_Options.settingsFile, &encrypt, "daysToKeep", &m_Options.daysToKeep, sizeof(m_Options.daysToKeep) );
   ReadValue( m_Options.settingsFile, &encrypt, "picsPerHour", &m_Options.picsPerHour, sizeof(m_Options.picsPerHour) );
   ReadValue( m_Options.settingsFile, &encrypt, "databasePath", m_Options.databasePath, sizeof(m_Options.databasePath) );
   ReadValue( m_Options.settingsFile, &encrypt, "databaseName", m_Options.databaseName, sizeof(m_Options.databaseName) );
   ReadValue( m_Options.settingsFile, &encrypt, "login", m_Options.login, sizeof(m_Options.login) );
   ReadValue( m_Options.settingsFile, &encrypt, "password", m_Options.password, sizeof(m_Options.password) );
   ReadValue( m_Options.settingsFile, &encrypt, "showInSystray", &m_Options.showInSystray, sizeof(m_Options.showInSystray) );
   ReadValue( m_Options.settingsFile, &encrypt, "screenCapSize", &m_Options.screenCapSize, sizeof(m_Options.screenCapSize) );
   ReadValue( m_Options.settingsFile, &encrypt, "showThumbnails", &m_Options.showThumbnails, sizeof(m_Options.showThumbnails) );
   ReadValue( m_Options.settingsFile, &encrypt, "allowRemoteAccess", &m_Options.allowRemoteAccess, sizeof(m_Options.allowRemoteAccess) );
   ReadValue( m_Options.settingsFile, &encrypt, "xScreenSize", &m_Options.xScreenSize, sizeof(m_Options.xScreenSize) );
   ReadValue( m_Options.settingsFile, &encrypt, "yScreenSize", &m_Options.yScreenSize, sizeof(m_Options.yScreenSize) );

   // restore the certificate
   m_Options.pCertificate = (unsigned char *) malloc( m_Options.certificateSize );
   ReadValue( m_Options.settingsFile, &encrypt, "SSLCertificate", m_Options.pCertificate, m_Options.certificateSize );

   // restore the private key
   m_Options.pPrivateKey = (unsigned char *) malloc( m_Options.privateKeySize );
   ReadValue( m_Options.settingsFile, &encrypt, "SSLPrivateKey", m_Options.pPrivateKey, m_Options.privateKeySize );

   ReadValue( m_Options.settingsFile, &encrypt, "email.lastFileId", &m_Options.email.lastFileId, sizeof(m_Options.email.lastFileId) );
   ReadValue( m_Options.settingsFile, &encrypt, "email.address", m_Options.email.address, sizeof(m_Options.email.address) );
   ReadValue( m_Options.settingsFile, &encrypt, "email.login", m_Options.email.login, sizeof(m_Options.email.login) );
   ReadValue( m_Options.settingsFile, &encrypt, "email.password", m_Options.email.password, sizeof(m_Options.email.password) );
   ReadValue( m_Options.settingsFile, &encrypt, "email.picsPerHour", &m_Options.email.picsPerHour, sizeof(m_Options.email.picsPerHour) );
   ReadValue( m_Options.settingsFile, &encrypt, "email.on", &m_Options.email.on, sizeof(m_Options.email.on) );

   ReadValue( m_Options.settingsFile, &encrypt, "productKey", &m_Options.productKey, sizeof(m_Options.productKey) );

   //a valid key means the install time will never be 15 days in the past
   if ( ProductKey::VerifyKey(&m_Options.productKey) )
   {
      m_Options.installTime.dwHighDateTime = INT_MAX;
      m_Options.installTime.dwLowDateTime  = INT_MAX;
   }

   return TRUE;
}

void OptionsControl::Update( void )
{
   if ( m_IsDirty )
   {
      Encryptor encrypt;

      if ( g_Core.GetAppIntegrityUserMode( )->DisableOptionsRegKey( ) )
      {
         // close the handle so we can modify this file
         CloseHandle( m_hSettingsFileHandle );

         WriteValue( m_Options.settingsFile, &encrypt, "Version", &Options::CurrentVersion, sizeof(Options::CurrentVersion) );
         WriteValue( m_Options.settingsFile, &encrypt, "privateKeySize", &m_Options.privateKeySize, sizeof(m_Options.privateKeySize) );
         WriteValue( m_Options.settingsFile, &encrypt, "certificateSize", &m_Options.certificateSize, sizeof(m_Options.certificateSize) );

         WriteValue( m_Options.settingsFile, &encrypt, "keyExpireTime", &m_Options.keyExpireTime, sizeof(m_Options.keyExpireTime) );
         WriteValue( m_Options.settingsFile, &encrypt, "days", m_Options.days, sizeof(m_Options.days) );
         WriteValue( m_Options.settingsFile, &encrypt, "port", &m_Options.port, sizeof(m_Options.port) );
         WriteValue( m_Options.settingsFile, &encrypt, "daysToKeep", &m_Options.daysToKeep, sizeof(m_Options.daysToKeep) );
         WriteValue( m_Options.settingsFile, &encrypt, "picsPerHour", &m_Options.picsPerHour, sizeof(m_Options.picsPerHour) );
         WriteValue( m_Options.settingsFile, &encrypt, "databasePath", m_Options.databasePath, sizeof(m_Options.databasePath) );
         WriteValue( m_Options.settingsFile, &encrypt, "databaseName", m_Options.databaseName, sizeof(m_Options.databaseName) );
         WriteValue( m_Options.settingsFile, &encrypt, "login", m_Options.login, sizeof(m_Options.login) );
         WriteValue( m_Options.settingsFile, &encrypt, "password", m_Options.password, sizeof(m_Options.password) );
         WriteValue( m_Options.settingsFile, &encrypt, "showInSystray", &m_Options.showInSystray, sizeof(m_Options.showInSystray) );
         WriteValue( m_Options.settingsFile, &encrypt, "screenCapSize", &m_Options.screenCapSize, sizeof(m_Options.screenCapSize) );
         WriteValue( m_Options.settingsFile, &encrypt, "showThumbnails", &m_Options.showThumbnails, sizeof(m_Options.showThumbnails) );
         WriteValue( m_Options.settingsFile, &encrypt, "allowRemoteAccess", &m_Options.allowRemoteAccess, sizeof(m_Options.allowRemoteAccess) );
         WriteValue( m_Options.settingsFile, &encrypt, "xScreenSize", &m_Options.xScreenSize, sizeof(m_Options.xScreenSize) );
         WriteValue( m_Options.settingsFile, &encrypt, "yScreenSize", &m_Options.yScreenSize, sizeof(m_Options.yScreenSize) );

         // save the certificate
         WriteValue( m_Options.settingsFile, &encrypt, "SSLCertificate", m_Options.pCertificate, m_Options.certificateSize );

         // save the private key
         WriteValue( m_Options.settingsFile, &encrypt, "SSLPrivateKey", m_Options.pPrivateKey, m_Options.privateKeySize );

         WriteValue( m_Options.settingsFile, &encrypt, "email.lastFileId", &m_Options.email.lastFileId, sizeof(m_Options.email.lastFileId) );
         WriteValue( m_Options.settingsFile, &encrypt, "email.address", m_Options.email.address, sizeof(m_Options.email.address) );
         WriteValue( m_Options.settingsFile, &encrypt, "email.login", m_Options.email.login, sizeof(m_Options.email.login) );
         WriteValue( m_Options.settingsFile, &encrypt, "email.password", m_Options.email.password, sizeof(m_Options.email.password) );
         WriteValue( m_Options.settingsFile, &encrypt, "email.picsPerHour", &m_Options.email.picsPerHour, sizeof(m_Options.email.picsPerHour) );
         WriteValue( m_Options.settingsFile, &encrypt, "email.on", &m_Options.email.on, sizeof(m_Options.email.on) );

         WriteValue( m_Options.settingsFile, &encrypt, "productKey", &m_Options.productKey, sizeof(m_Options.productKey) );

         m_IsDirty = FALSE;
         g_Core.GetAppIntegrityUserMode( )->EnableOptionsRegKey( );

         m_hSettingsFileHandle = CreateFile( m_Options.settingsFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
      }
   }
}

uint32 OptionsControl::GetTotalHoursToRecord( void )
{
   uint32 totalHours = 0;

   uint32 i;
   for ( i = 0; i < 7; i++ )
   {
      uint32 c;
      for ( c = 0; c < 4; c++ )
      {
         if ( m_Options.days[ i ].periods[ c ] )
         {
            totalHours += 6;
         }
      }
   }

   return totalHours;
}

BOOL OptionsControl::ReadValue(
   const char *pSettingsFile,
   Encryptor *pEncrypt,
   const char *pKey,
   void *pValue,
   int valueSize
)
{
   EncryptResultScope scope = pEncrypt->AllocEncrypted(valueSize);
   if ( FALSE == GetPrivateProfileStruct( OptionsControl::AppName, pKey, scope.GetResult( ).GetBuffer( ), scope.GetResult( ).GetSize( ), pSettingsFile ) )
   {
      return FALSE;
   }

   EncryptResultScope result = pEncrypt->Decrypt(scope.GetResult( ).GetBuffer( ), scope.GetResult( ).GetSize( ));
   memcpy( pValue, result.GetResult( ).GetBuffer( ), valueSize );

   return TRUE;
}

BOOL OptionsControl::WriteValue(
   const char *pSettingsFile,
   Encryptor *pEncrypt,
   const char *pKey,
   const void *pValue,
   int valueSize
)
{
   EncryptResultScope scope = pEncrypt->Encrypt(pValue, valueSize);

   WritePrivateProfileStruct( OptionsControl::AppName, pKey, scope.GetResult( ).GetBuffer( ), scope.GetResult( ).GetSize( ), pSettingsFile );
   
   return TRUE;
}
