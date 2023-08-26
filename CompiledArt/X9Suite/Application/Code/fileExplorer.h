
#ifndef FILE_EXPLORER_H_
#define FILE_EXPLORER_H_

// this is our database manager
#include "Database.h"
#include "commandBuffer.h"
#include "options.h"
      
static const char * FILE_EXPLORER_HEADER_NAME = "Header";
static const uint32 JPEG_COMPRESSION_RATE     = 4;

typedef struct _X9ImageHeader
{
   sint32 width;
   sint32 height;
   uint32 uniqueID;
}
X9ImageHeader;

class FileExplorer : public ICommand
{
   public:
      typedef enum _Error
      {
         Error_None,
         Error_DiskSpace
      }
      Error;

      FileExplorer ( void );
      ~FileExplorer( );

      void Create( void );

      BOOL OpenDatabase ( void );
      void CloseDatabase( void );
      
      Error ResizeDatabase( void );

      BOOL AddStream ( IStream *pStream, sint16 width, sint16 height );
      void DeleteFile( sint32 fileIndex );

      BOOL GetFile     ( sint32 index, void **ppBuffer );
      BOOL GetThumbnail( sint32 index, void **ppBuffer, uint32 *pThumbSize );

      sint32 GetFileSize        ( sint32 index );
      sint32 GetFileWidth       ( sint32 index );
      sint32 GetFileHeight      ( sint32 index );
      uint32 GetFileUniqueID    ( sint32 index );
      uint32 GetNextFileUniqueID( void );
      float  GetFileAspectRatio ( sint32 index );
      void   GetFileName        ( sint32 fileIndex, char *pFileName );

      sint32 GetFirstFileIndex ( void );
      sint32 GetPrevIndex      ( sint32 currentIndex );
      sint32 GetNextIndex      ( sint32 currentIndex );
      sint32 GetNumFiles       ( void );
      sint32 GetMaxFiles       ( void );
      sint32 GetMaxFileSize    ( void );

      uint32 GetFileIndexByUniqueID( sint32 uniqueID );

      uint32 CalcDatabaseSize  ( sint32 maxFiles, ScreenCapSize screenCapSize );

      BOOL CheckForCorruptDatabase( void );

      void DumpDatabase( void );
      BOOL DumpToFile  ( char *pFileName, sint32 index );

      void ExecuteCommand( ICommand::Command *pCommand );
      const char * GetCommandName( void );

      float *StartProgressBarDisplay( const char *pProgressInfo );
      void   StopProgressBarDisplay ( void );

   private:
      typedef struct _X9FileExplorerHeader
      {
         sint32   m_FirstFileIndex;
         sint32   m_FileCount;

         uint32   m_UniqueID;

         sint32   m_MaxAllowedFiles;
         sint32   m_MaxFileSize;

         sint32   m_RealHeaderSize;

         sint32  *m_pFileList;
      }
      X9FileExplorerHeader;

      static   LRESULT  Callback                  ( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
      static   BOOL     FileExplorer_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam );
               BOOL     FileExplorer_WM_INITDIALOG( HWND hwnd );
               BOOL     FileExplorer_WM_DESTROY   ( HWND hwnd );
         
      static   DWORD WINAPI   ThreadProc( LPVOID pThreadArg );

      void CreateFileName( char *pFileName );
      BOOL CreateNewDatabase( void );
      
      BOOL SaveHeader          ( Database *pDatabase, X9FileExplorerHeader *pHeader, BYTE *pHeaderBuffer );
      BOOL LoadHeader          ( void );
      void AllocateHeader      ( X9FileExplorerHeader **ppHeader, BYTE **ppHeaderBuffer, Database *pDatabase );
      void SetupFileListPointer( X9FileExplorerHeader *pHeader, BYTE *pHeaderBuffer );
      BOOL IsDiskSpaceAvailable( uint32 maxFiles, uint32 averageSectorsPerFile );
      
      uint32 CalcAverageSectorsPerFile( ScreenCapSize screenCapSize );

      HWND                  m_Hwnd;
      HANDLE                m_hThread;
      char                  m_ProgressInfoStr[ MAX_PATH ];
      float                 m_DatabaseProgress; //the progress completed in generating a database

      Database              m_Database;

      X9FileExplorerHeader *m_pHeader;
      BYTE                 *m_pHeaderBuffer; //this is what we'll pass to the database to write

      sint32               *m_pUnpackFileList; //Used when we need to unwrap the file list because of a delete.
      
      BYTE                 *m_pScratchFileBuffer;
};

#endif
