
#ifndef OPTIONS_H_
#define OPTIONS_H_

#include "productKey.h"

#define APP_NAME       "Second Look"
#define COMPANY_NAME   "Compiled Art"
#define COMPANY_HOME   "www.compiledart.com"
#define TIMESTAMP_NAME "CaSl.dlu"
#define SETTINGS_NAME  "settings.dat"

class Registry_Class;
class Encryptor;

static const uint32 DEF_THUMB_WIDTH     = 100;
static const uint32 MAX_PICS_PER_HOUR   = 60;
static const uint32 MAX_PERIODS_PER_DAY = 4;

typedef enum _ScreenCapSize
{
   ScreenCapSize_Full,
   ScreenCapSize_Medium,
   ScreenCapSize_Small,
   ScreenCapSize_Total
}
ScreenCapSize;

typedef struct _Day
{
   char periods[ MAX_PERIODS_PER_DAY ];
}
Day;

typedef struct _EmailSettings
{
   uint64 lastFileId;
   
   char   address[ 256 ];
   char   login[ 256 ];
   char   password[ 256 ];
   char   picsPerHour;
   char   on;
}
EmailSettings;

//This is a seperate struct so it's easier to read/write from a file
typedef struct _Options
{
   static const uint32 CurrentVersion = 1;

   ProductKey::Key productKey;

   FILETIME keyExpireTime;
   FILETIME installTime;

   Day days[ 7 ];
   EmailSettings email;

   unsigned char *pPrivateKey;
   unsigned char *pCertificate;

   uint32 privateKeySize;
   uint32 certificateSize;
   sint16 port;
   sint16 daysToKeep;
   char   showThumbnails;
   char   picsPerHour;  
   char   allowRemoteAccess;

   char   databasePath[ MAX_PATH ];
   char   databaseName[ MAX_PATH ];
   char   settingsFile[ MAX_PATH ];
   char   login[ 32 ];
   char   password[ 32 ];
   char   showInSystray;

   uint32 xScreenSize;
   uint32 yScreenSize;

   ScreenCapSize screenCapSize;
}
Options;

class OptionsControl
{
   public:

            OptionsControl   ( void );
            ~OptionsControl  ( );

      void  Create           ( void );
      void  SetDefaultOptions( void );

      void  Update           ( void );

      uint32 GetTotalHoursToRecord( void );

   private:
      BOOL  LoadOptions      ( void );

   public:
      Options m_Options;
      BOOL    m_IsDirty;

   private:
      HANDLE  m_hSettingsFileHandle;
   
   public:

      static const short DefaultWebPort = 443;
      static const char  CompanyDomain[];
      static const char  RegisterPage[];
      static const char  SupportPage[];
      static const char  AppName[];
      static const char  CompanyName[];
      static const char  OptionsRegKey[];
      static const char  RegistryRunValue[];
      static const char  RunOnceMutex[];
      
      static const float sScreenCapSize[ ScreenCapSize_Total ];


   private:
      BOOL ReadValue (const char *pSettingsFile, Encryptor *pEncrypt, const char *pKey, void *pValue, int valueSize);
      BOOL WriteValue(const char *pSettingsFile, Encryptor *pEncrypt, const char *pKey, const void *pValue, int valueSize);
};

#endif
