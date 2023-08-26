
#include "precompiled.h"
#include "fileExplorer.h"
#include "main.h"
#include "utilityClock.h"

FileExplorer::FileExplorer( void )
{
   m_pHeader            = NULL;
   m_pHeaderBuffer      = NULL;
   m_pUnpackFileList    = NULL;
   m_pScratchFileBuffer = NULL;

   m_hThread            = NULL;
   m_DatabaseProgress   = 0.00f;

   memset( m_ProgressInfoStr, 0, sizeof( m_ProgressInfoStr ) );
}

FileExplorer::~FileExplorer( )
{
   CloseDatabase( );
}

void FileExplorer::Create( void )
{
   g_CommandBuffer.Register( this );
}

LRESULT FileExplorer::Callback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   FileExplorer *pFileExplorer = (FileExplorer *)ULongToPtr( GetWindowLong( hwnd, GWLP_USERDATA ) );

   if ( pFileExplorer || WM_INITDIALOG == uMsg )
   {
      switch( uMsg )
      {
         HANDLE_DLGMSG( hwnd, WM_INITDIALOG, FileExplorer::FileExplorer_WM_INITDIALOG );
         HANDLE_DLGMSG( hwnd, WM_DESTROY   , pFileExplorer->FileExplorer_WM_DESTROY   );
      }
   }

   return FALSE;
}

BOOL FileExplorer::FileExplorer_WM_INITDIALOG( HWND hwnd, HWND hwndFocus, LPARAM lParam )
{
   FileExplorer *pFileExplorer = (FileExplorer *)lParam;

   SetWindowLong( hwnd, GWLP_USERDATA, PtrToLong(pFileExplorer) );

   return pFileExplorer->FileExplorer_WM_INITDIALOG( hwnd );
}

BOOL FileExplorer::FileExplorer_WM_INITDIALOG( HWND hwnd )
{
   //Setup the progress bar
   SendMessage( GetDlgItem( hwnd, ID_PROGRESS_DB_CREATE ), PBM_SETRANGE, 0, MAKELPARAM( 0, 100 ) );

   // disable the X button and close on the system menu. We will control exiting.
   HMENU hMenu = GetSystemMenu( hwnd, FALSE );

   EnableMenuItem( hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED | MF_DISABLED );

   DrawMenuBar( hwnd );
   // done

   // set our icon
   HICON hIcon = LoadIcon( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_ICON_X9 ) );

   SendMessage( hwnd, WM_SETICON, ICON_BIG  , (LPARAM) hIcon );
   SendMessage( hwnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon );

   ShowWindow( hwnd, SW_SHOWNORMAL );

   return TRUE;
}

BOOL FileExplorer::FileExplorer_WM_DESTROY( HWND hwnd )
{
   ShowWindow( hwnd, SW_HIDE );

   return TRUE;
}

float *FileExplorer::StartProgressBarDisplay( const char *pProgressInfo )
{
   // reset the database progress
   m_DatabaseProgress = 0.00f;

   // copy the info into a buffer the thread can read
   strcpy( m_ProgressInfoStr, pProgressInfo );

   m_DatabaseProgress = 0.0f;

   // spawn a thread that can update the progress bar...
   DWORD threadId;
   m_hThread = CreateThread( NULL, 0, ThreadProc, this, NULL, &threadId );

   return &m_DatabaseProgress;
}

void FileExplorer::StopProgressBarDisplay( void )
{
   // force the database progress to 100% so we end.
   m_DatabaseProgress = 1.00f;

   // wait for the thread proc to terminate
   WaitForSingleObject( m_hThread, INFINITE );

   // now close the handle; we're done with the thread
   CloseHandle( m_hThread );
}

DWORD WINAPI FileExplorer::ThreadProc( LPVOID pThreadArg )
{
   FileExplorer *pFileExplorer = (FileExplorer *)pThreadArg;

   // create a window we'll use to display progress on database creation/resizing
   pFileExplorer->m_Hwnd = CreateDialogParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( ID_DIALOG_DB_PROGRESS ), NULL, (DLGPROC)Callback, (LPARAM) pFileExplorer );
   ShowWindow( pFileExplorer->m_Hwnd, SW_SHOW );
   SetForegroundWindow( pFileExplorer->m_Hwnd );

   // set the text for the progres bar info
   SetWindowText( GetDlgItem( pFileExplorer->m_Hwnd, ID_STATIC_PROGRESS_INFO ), pFileExplorer->m_ProgressInfoStr );

   // set the progress bar in front of all other windows
   SetWindowPos( pFileExplorer->m_Hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );

   float progressUpdateTimer = 0.00;
   while( pFileExplorer->m_DatabaseProgress < 1.00f )
   {
      if ( progressUpdateTimer < Timer::m_total )
      {
         // we don't need a mutex around databaseProgress because we're simply reading its value
         // to update a progress bar. Nothing that could crash
         uint32 percentComplete = (uint32) (pFileExplorer->m_DatabaseProgress * 100.00f);

         PostMessage( GetDlgItem( pFileExplorer->m_Hwnd, ID_PROGRESS_DB_CREATE ), PBM_SETPOS, percentComplete, 0 );

         // update twice a second; we don't want to spam it with updates.
         progressUpdateTimer = Timer::m_total + .50f;
      }

      // update the global timer and message pump so our UI runs
      Timer::Update( );

      MSG msg;
      if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
      {  
         TranslateMessage( &msg );
         DispatchMessage( &msg );
      }
   }

   // clean up the window
   DestroyWindow( pFileExplorer->m_Hwnd );

   pFileExplorer->m_Hwnd = NULL;

   return 0;
}

BOOL FileExplorer::CreateNewDatabase( void )
{
   BOOL success = FALSE;

   do
   {
      // calculate the max amount of files we'll need
      uint32 maxFiles = g_Options.m_Options.picsPerHour ? g_Options.m_Options.picsPerHour : 1;
      maxFiles *= g_Options.GetTotalHoursToRecord( );

      // and the average amount of sectors per file
      uint32 averageSectorsPerFile = CalcAverageSectorsPerFile( g_Options.m_Options.screenCapSize );
      if ( !averageSectorsPerFile ) break;

      // create our file path
      char filePath[ MAX_PATH ];
      sprintf( filePath, "%s%s", g_Options.m_Options.databasePath, g_Options.m_Options.databaseName );

      // make sure the directory exists
      CreateDirectory( g_Options.m_Options.databasePath, NULL );

      // test for available HD space
      if ( !IsDiskSpaceAvailable( maxFiles, averageSectorsPerFile ) )
      {
         break;
      }

      StartProgressBarDisplay( "Please wait while Second Look generates the image database for the first time.\r\n"
                               "This may take several minutes depending on your computer's configuration." );

      // then create the database. Add one extra picture for our data
      m_Database.CreateDatabaseFile( filePath, maxFiles + 1, averageSectorsPerFile, &m_DatabaseProgress );

      StopProgressBarDisplay( );

      // open a handle to it
      if ( !m_Database.OpenDatabase( filePath ) ) break;

      // create our header
      AllocateHeader( &m_pHeader, &m_pHeaderBuffer, &m_Database );

      SetupFileListPointer( m_pHeader, m_pHeaderBuffer );

      m_pHeader->m_MaxAllowedFiles = m_Database.GetMaxNumFiles( ) - 1;
      m_pHeader->m_MaxFileSize     = m_Database.GetMaxFileSize( );

      m_pUnpackFileList    = new int[ m_pHeader->m_MaxAllowedFiles ];
      m_pScratchFileBuffer = new BYTE[ m_pHeader->m_MaxFileSize ];
      
      // save our header so it gets in the first slot
      if ( !SaveHeader( &m_Database, m_pHeader, m_pHeaderBuffer ) ) break;

      CloseDatabase( );

      OpenDatabase( );

      success = TRUE;
   }
   while( 0 );

   return success;
}

void FileExplorer::AllocateHeader( X9FileExplorerHeader **ppHeader, BYTE **ppHeaderBuffer, Database *pDatabase )
{
   // setup our sizes
   X9FileExplorerHeader headerForSize;
   uint32 realHeaderSize = sizeof( headerForSize ) + sizeof( int ) * pDatabase->GetMaxNumFiles( );
   
   // allocate and clear our buffer
   *ppHeaderBuffer = new BYTE[ realHeaderSize ];
   memset( *ppHeaderBuffer, 0, realHeaderSize );

   //setup values
   *ppHeader = (X9FileExplorerHeader *)*ppHeaderBuffer;
   
   (*ppHeader)->m_RealHeaderSize = realHeaderSize;

   (*ppHeader)->m_FirstFileIndex = 0;
   (*ppHeader)->m_FileCount      = 0;
   (*ppHeader)->m_UniqueID       = 0;
   (*ppHeader)->m_pFileList      = NULL; //This must be setup outside allocate header
}

void FileExplorer::SetupFileListPointer( X9FileExplorerHeader *pHeader, BYTE *pHeaderBuffer )
{
   // setup our filelist pointer, which comes at the end of the header in our buffer
   X9FileExplorerHeader headerForSize;
   pHeader->m_pFileList = (sint32 *) ((BYTE *)pHeaderBuffer + sizeof( headerForSize ));
}

BOOL FileExplorer::IsDiskSpaceAvailable( uint32 maxFiles, uint32 averageSectorsPerFile )
{
   ULARGE_INTEGER freeSpaceInBytes;
   ULARGE_INTEGER totalSpaceInBytes;
   if ( GetDiskFreeSpaceEx( g_Options.m_Options.databasePath, &freeSpaceInBytes, &totalSpaceInBytes, NULL ) )
   {
      uint64 requiredSpace = averageSectorsPerFile * maxFiles * SECTOR_SIZE;
      if ( freeSpaceInBytes.QuadPart >= requiredSpace )
      {
         return TRUE;
      }
      
      g_Log.Write( Log::TypeError, "Not enough free disk space for database. %d available, need %d\n", freeSpaceInBytes.QuadPart, requiredSpace );
   }
   else
   {
      g_Log.Write( Log::TypeError, "GetDiskFreeSpaceEx failed.\n" );
   }
   return FALSE;
}

uint32 FileExplorer::CalcAverageSectorsPerFile( ScreenCapSize screenCapSize )
{
   uint32 xScreenSize = GetSystemMetrics( SM_CXVIRTUALSCREEN );
   uint32 yScreenSize = GetSystemMetrics( SM_CYVIRTUALSCREEN );
   if ( !xScreenSize || !yScreenSize )
   {
      g_Log.Write( Log::TypeError, "Failed to get screen resolution for CalcAverageSectorsPerFile.\n" );
      return 0;
   }

   float aspectRatio = (float) xScreenSize / (float) yScreenSize;

   float imageWidth;
   if ( ScreenCapSize_Full == screenCapSize )
   {
      imageWidth = (float) xScreenSize;
   }
   else
   {
      imageWidth = g_Options.sScreenCapSize[ screenCapSize ];
   }

   float imageHeight = imageWidth / aspectRatio;
   float imageArea   = imageWidth * imageHeight;

   // divide the image area (which is in bytes) by an estimate of how well JPEG will compress. 
   // Then convert to sectors.
   return ((uint32) imageArea / JPEG_COMPRESSION_RATE) / SECTOR_SIZE;
}

uint32 FileExplorer::CalcDatabaseSize( sint32 maxFiles, ScreenCapSize screenCapSize )
{
   // first get the average sectors we'll need per file
   uint32 sectorsPerFile = CalcAverageSectorsPerFile( screenCapSize );
   
   // convert to filesize...
   uint32 averageFileSize = sectorsPerFile * SECTOR_SIZE;

   // and multiply by our files
   return maxFiles * averageFileSize;
}

BOOL FileExplorer::OpenDatabase( void )
{
   BOOL success         = FALSE;
   BOOL needNewDatabase = FALSE;

   char filePath[ MAX_PATH ];
   sprintf( filePath, "%s%s", g_Options.m_Options.databasePath, g_Options.m_Options.databaseName );

   do
   {
      // first test to see if there's an existing database
      if ( !m_Database.DoesDatabaseFileExist( filePath ) )
      {
         needNewDatabase = TRUE;
         break;
      }

      // open the database, and then load our header
      if  ( !m_Database.OpenDatabase( filePath ) )
      {
         needNewDatabase = TRUE;
         break;
      }     
      
      if ( !LoadHeader( ) )
      {
         // uh oh, we failed to load the header, we have a corrupt database.
         m_Database.CloseDatabase( );

         // recreate the database
         MessageBox( GetForegroundWindow( ), "We are very sorry, the image database has become corrupt and needs to be recreated.", APP_NAME, MB_ICONINFORMATION | MB_OK );
         
         // delete the database
         ::DeleteFile( filePath );

         needNewDatabase = TRUE;
         break;
      }

      m_pUnpackFileList    = new int[ m_pHeader->m_MaxAllowedFiles ];
      m_pScratchFileBuffer = new BYTE[ m_pHeader->m_MaxFileSize ];

      success = TRUE;
   }
   while( 0 );

   // if needing a new database was flagged, attempt to create it
   if ( needNewDatabase )
   {
      // if that goes ok, all is well.
      if ( CreateNewDatabase( ) )
      {
         success = TRUE;
      }
   }

   return success;
}

BOOL FileExplorer::CheckForCorruptDatabase( void )
{
   if ( m_Database.IsCorrupt( ) )
   {
      // uh oh, we failed to load the header, we have a corrupt database.
      m_Database.CloseDatabase( );

      // recreate the database
      MessageBox( GetForegroundWindow( ), "We are very sorry, the image database has become corrupt and needs to be recreated.", APP_NAME, MB_ICONINFORMATION | MB_OK );
      
      char filePath[ MAX_PATH ];
      sprintf( filePath, "%s%s", g_Options.m_Options.databasePath, g_Options.m_Options.databaseName );

      // delete the database
      ::DeleteFile( filePath );

      // if THAT fails, we've really got a problem.
      if ( !CreateNewDatabase( ) )
      {
         return FALSE;
      }
   }

   return TRUE;
}    

void FileExplorer::CloseDatabase( void )
{
   m_Database.CloseDatabase( );

   delete [] m_pHeaderBuffer;
   delete [] m_pUnpackFileList;
   delete [] m_pScratchFileBuffer;

   m_pHeaderBuffer      = NULL;
   m_pUnpackFileList    = NULL;
   m_pScratchFileBuffer = NULL;
}

FileExplorer::Error FileExplorer::ResizeDatabase( void )
{
   Database newDatabase;
   BYTE *pNewHeaderBuffer    = NULL;
   int *pSectorList          = NULL;
   FileExplorer::Error error = FileExplorer::Error_None;

   do
   {
      char filePath[ MAX_PATH ];
      sprintf( filePath, "%s%s", g_Options.m_Options.databasePath, g_Options.m_Options.databaseName );

      // it better exist
      if ( !m_Database.DoesDatabaseFileExist( filePath ) ) break;
         
      // attempt to create the new database
         
      // test for available HD space
      uint32 sectorsPerFile = CalcAverageSectorsPerFile( g_Options.m_Options.screenCapSize );
      if ( !sectorsPerFile ) break;

      int pics = g_Options.m_Options.picsPerHour ? g_Options.m_Options.picsPerHour : 1;

      uint32 totalFiles = g_Options.GetTotalHoursToRecord( ) * pics;

      if ( !IsDiskSpaceAvailable( totalFiles, sectorsPerFile ) ) { error = FileExplorer::Error_DiskSpace; break; }

      char newDatabasePath[ MAX_PATH ];
      sprintf( newDatabasePath, "%s%sclone", g_Options.m_Options.databasePath, g_Options.m_Options.databaseName );

      StartProgressBarDisplay( "Second Look needs to resize the image database to maintain image history.\r\n"
                               "This may take several minutes depending on your computer's configuration." );

      // make sure to add one to the database so we have room for our header
      newDatabase.CreateDatabaseFile( newDatabasePath, totalFiles + 1, sectorsPerFile, &m_DatabaseProgress );

      StopProgressBarDisplay( );

      // open a handle to it
      if ( !newDatabase.OpenDatabase( newDatabasePath ) ) break;

      // create our header
      X9FileExplorerHeader *pNewHeader;
      AllocateHeader( &pNewHeader, &pNewHeaderBuffer, &newDatabase );

      SetupFileListPointer( pNewHeader, pNewHeaderBuffer );

      pNewHeader->m_MaxAllowedFiles = newDatabase.GetMaxNumFiles( ) - 1;
      pNewHeader->m_MaxFileSize     = newDatabase.GetMaxFileSize( );
      
      // save our header so it gets in the first slot
      if ( !SaveHeader( &newDatabase, pNewHeader, pNewHeaderBuffer ) ) break;

      // now we need to copy the contents of the existing database into the new one.
      // that might not actually be to bad. In a loop, request each file out of the current database,
      // and then just save it to the new one.
      
      // make a buffer large enough to store our worst case
      pSectorList = new int[ sectorsPerFile * 2 ];

      sint32 readIndex = m_pHeader->m_FirstFileIndex;
      sint32 i;
      for ( i = 0; i < m_pHeader->m_FileCount; i++ )
      {
         // grab each file from the database
         sint32 fileSize = m_Database.GetFileSize( m_pHeader->m_pFileList[ i ] );

         char fileName[ MAX_PATH ];
         m_Database.GetFileName( m_pHeader->m_pFileList[ i ], fileName );

         m_Database.GetFile( m_pHeader->m_pFileList[ i ], m_pScratchFileBuffer );

         int sectorsRequired = newDatabase.GetRequiredSectorCount( fileSize );

         newDatabase.FindFreeSectors( sectorsRequired, pSectorList );
         int imageIndex = newDatabase.AddFile( m_pScratchFileBuffer, fileName, fileSize, pSectorList );
         if ( -1 == imageIndex ) break;

         // add to our new header list
         pNewHeader->m_pFileList[ pNewHeader->m_FileCount ] = imageIndex;
         pNewHeader->m_FileCount++;
         pNewHeader->m_UniqueID++;

         // move to then next file in the old database
         readIndex = ( readIndex + 1 ) % m_pHeader->m_FileCount;
      }

      if ( i != m_pHeader->m_FileCount ) break;

      // now all files should be in the new database, so
      // let's save the header and load this database as our own.
      SaveHeader( &newDatabase, pNewHeader, pNewHeaderBuffer );

      newDatabase.CloseDatabase( );

      // close our old database
      CloseDatabase( );

      // move/rename our cloned file on top of our existing database.
      MoveFileEx( newDatabasePath, filePath, MOVEFILE_REPLACE_EXISTING );

      // and now open the newly created one
      OpenDatabase( );
   }
   while( 0 );

   if ( pNewHeaderBuffer )
   {
      delete [] pNewHeaderBuffer;
   }

   if ( pSectorList )
   {
      delete [] pSectorList;
   }

   return error;
}

BOOL FileExplorer::SaveHeader( Database *pDatabase, X9FileExplorerHeader *pHeader, BYTE *pHeaderBuffer )
{
   BOOL success     = FALSE;
   BYTE *pBuffer    = NULL;
   int *pSectorList = NULL;
   
   do
   {
      //Clock clock;
      //clock.Start( );

      // delete the header if it exists
      pDatabase->DeleteFile( FILE_EXPLORER_HEADER_NAME, FALSE );
      
      //_RPT1( _CRT_WARN, "\t\tDeleteFile: %.4f\n", clock.MarkSample( ) );
      
      // how many sectors will this file need?
      int requiredSectors = pDatabase->GetRequiredSectorCount( pHeader->m_RealHeaderSize );
      pSectorList = new int[ requiredSectors ];

      //_RPT1( _CRT_WARN, "\t\tAllocSectors: %.4f\n", clock.MarkSample( ) );

      if ( !pDatabase->FindFreeSectors( requiredSectors, pSectorList ) ) break;

      //_RPT1( _CRT_WARN, "\t\tFindFreeSectors: %.4f\n", clock.MarkSample( ) );

      int imageIndex = pDatabase->AddFile( pHeaderBuffer, FILE_EXPLORER_HEADER_NAME, pHeader->m_RealHeaderSize, pSectorList );

      //_RPT1( _CRT_WARN, "\t\tAddFiles: %.4f\n", clock.MarkSample( ) );

      delete [] pSectorList;

      if ( -1 == imageIndex ) break;

      success = TRUE;
   }
   while( 0 );

   return success;
}

BOOL FileExplorer::LoadHeader( void )
{
   AllocateHeader( &m_pHeader, &m_pHeaderBuffer, &m_Database );

   // watch out for a failed header load. That'd be a big problem
   if ( m_Database.GetFile( FILE_EXPLORER_HEADER_NAME, m_pHeaderBuffer ) != -1 )
   {
      // now that we've loaded, we can set our file list pointer to our file list.
      SetupFileListPointer( m_pHeader, m_pHeaderBuffer );

      return TRUE;
   }
   else
   {
      // clean up our header buffer since we failed to load.
      delete [] m_pHeaderBuffer;
   }

   return FALSE;
}

void FileExplorer::CreateFileName( char *pFileName )
{
   SYSTEMTIME systemTime;
   GetLocalTime( &systemTime );

   _snprintf( pFileName, 26, "%d-%d-%d-%d-%d-%d-%d-%d", systemTime.wDayOfWeek, systemTime.wMonth, systemTime.wDay, systemTime.wYear, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds );
   pFileName[ 29 ] = 0;
}

BOOL FileExplorer::AddStream( IStream *pStream, sint16 width, sint16 height )
{
   BOOL success     = FALSE;
   BYTE *pBuffer    = NULL;
   int *pSectorList = NULL;

   do
   {
      // get the size of the stream
      STATSTG statStg = { 0 };
      if ( pStream->Stat( &statStg, STATFLAG_NONAME ) != S_OK ) break;

      // we want our file to store the dimensions as well, so increase the size by 8.
      sint32 fileSize  = (sint32) statStg.cbSize.QuadPart;
      sint32 imageSize = fileSize;
      fileSize += sizeof( X9ImageHeader );

      if ( fileSize > m_pHeader->m_MaxFileSize )
      {
         g_Log.Write( Log::TypeError, "File size too large: %d, max is %d", fileSize, m_pHeader->m_MaxFileSize );
         break;
      }

      // are we full?
      if ( m_pHeader->m_MaxAllowedFiles == m_pHeader->m_FileCount )
      {
         // then we first need to delete the oldest file
         DeleteFile( m_pHeader->m_FirstFileIndex );
      }

      // seek to the beginning of the stream
      LARGE_INTEGER largeInt = { 0 };
      if ( pStream->Seek( largeInt, SEEK_SET, NULL ) != S_OK ) break;

      // create a buffer and copy the stream's buffer into it, and the file dimensions
      pBuffer = new BYTE[ fileSize ];
      memset( pBuffer, 0, fileSize );

      X9ImageHeader *pHeader = (X9ImageHeader *)pBuffer;
      
      pHeader->width    = width;
      pHeader->height   = height;
      pHeader->uniqueID = m_pHeader->m_UniqueID++; //unique IDs are just throwaway numbers that can wrap

      if ( pStream->Read( pBuffer + sizeof( X9ImageHeader ), imageSize, NULL ) != S_OK ) break;

      // how many sectors will this file need?
      int requiredSectors = m_Database.GetRequiredSectorCount( fileSize );
      pSectorList = new int[ requiredSectors ];
      if ( !m_Database.FindFreeSectors( requiredSectors, pSectorList ) ) break;

      // add it to our database
      char fileName[ MAX_PATH ];
      CreateFileName( fileName );
      
      int imageIndex = m_Database.AddFile( pBuffer, fileName, fileSize, pSectorList );
      if ( -1 == imageIndex ) break;

      // again, are we full?
      if ( m_pHeader->m_MaxAllowedFiles == m_pHeader->m_FileCount )
      {
         // then store the file in the slot we deleted above
         m_pHeader->m_pFileList[ m_pHeader->m_FirstFileIndex ] = imageIndex;
         m_pHeader->m_FirstFileIndex = (m_pHeader->m_FirstFileIndex + 1) % m_pHeader->m_MaxAllowedFiles;
      }
      else
      {
         // not full, so just continue to populate our list
         m_pHeader->m_pFileList[ m_pHeader->m_FileCount ] = imageIndex;
         m_pHeader->m_FileCount++;
      }

      if ( !SaveHeader( &m_Database, m_pHeader, m_pHeaderBuffer ) ) break;

      success = TRUE;
   }
   while( 0 );

   if ( pBuffer )
   {
      delete [] pBuffer;
   }

   if ( pSectorList )
   {
      delete [] pSectorList;
   }

   return success;
}

void FileExplorer::DeleteFile( sint32 fileIndex )
{
   // ok, first get the filename out of the database
   //Clock clock;
   //clock.Start( );

   char fileName[ MAX_PATH ];
   m_Database.GetFileName( m_pHeader->m_pFileList[ fileIndex ], fileName );
   
   //_RPT1( _CRT_WARN, "\tGetFileName: %.4f\n", clock.MarkSample( ) );

   // next delete said file
   m_Database.DeleteFile( fileName, TRUE );

   //_RPT1( _CRT_WARN, "\tDelete: %.4f\n", clock.MarkSample( ) );

   if ( fileIndex < m_pHeader->m_FirstFileIndex )
   {
      // copy from firstFileIndex to the end of the array
      memcpy( m_pUnpackFileList, m_pHeader->m_pFileList + m_pHeader->m_FirstFileIndex, (m_pHeader->m_MaxAllowedFiles - m_pHeader->m_FirstFileIndex) * sizeof( int ) );

      // copy everything up to the file to delete
      memcpy( m_pUnpackFileList + (m_pHeader->m_MaxAllowedFiles - m_pHeader->m_FirstFileIndex), m_pHeader->m_pFileList, fileIndex * sizeof( int ) );

      // copy everything between file to delete and firstFileIndex
      memcpy( m_pUnpackFileList + (m_pHeader->m_MaxAllowedFiles - m_pHeader->m_FirstFileIndex) + fileIndex, 
              m_pHeader->m_pFileList + fileIndex + 1, 
              (m_pHeader->m_FirstFileIndex - fileIndex - 1) * sizeof( int ) );

      //_RPT1( _CRT_WARN, "\tcopy1: %.4f\n", clock.MarkSample( ) );
   }
   else if ( fileIndex > m_pHeader->m_FirstFileIndex )
   {
      // copy everything between firstFileIndex and the file to delete.
      memcpy( m_pUnpackFileList, m_pHeader->m_pFileList + m_pHeader->m_FirstFileIndex, (fileIndex - m_pHeader->m_FirstFileIndex) * sizeof( int ) );

      // copy everything after file to delete to the end of the array
      memcpy( m_pUnpackFileList + (fileIndex - m_pHeader->m_FirstFileIndex), m_pHeader->m_pFileList + fileIndex + 1, (m_pHeader->m_MaxAllowedFiles - fileIndex - 1) * sizeof( int ) );

      // copy the start of the array up to firstFileIndex
      memcpy( m_pUnpackFileList + (m_pHeader->m_MaxAllowedFiles - m_pHeader->m_FirstFileIndex - 1), m_pHeader->m_pFileList, m_pHeader->m_FirstFileIndex * sizeof( int ) );

      //_RPT1( _CRT_WARN, "\tcopy2: %.4f\n", clock.MarkSample( ) );
   }
   else
   {
      // copy everything from just after firstFileIndex to the end
      memcpy( m_pUnpackFileList, m_pHeader->m_pFileList + m_pHeader->m_FirstFileIndex + 1, (m_pHeader->m_MaxAllowedFiles - m_pHeader->m_FirstFileIndex - 1) * sizeof( int ) );

      memcpy( m_pUnpackFileList + (m_pHeader->m_MaxAllowedFiles - m_pHeader->m_FirstFileIndex - 1), m_pHeader->m_pFileList, m_pHeader->m_FirstFileIndex * sizeof( int ) );

      //_RPT1( _CRT_WARN, "\tcopy3: %.4f\n", clock.MarkSample( ) );
   }

   // now copy the scratch buffer back over
   memcpy( m_pHeader->m_pFileList, m_pUnpackFileList, m_pHeader->m_MaxAllowedFiles * sizeof( int ) );
   m_pHeader->m_FirstFileIndex = 0;
   m_pHeader->m_FileCount--;

   //_RPT1( _CRT_WARN, "\tscratch: %.4f\n", clock.MarkSample( ) );

   SaveHeader( &m_Database, m_pHeader, m_pHeaderBuffer );

   //_RPT1( _CRT_WARN, "\tsave header: %.4f\n", clock.MarkSample( ) );
}

BOOL FileExplorer::GetFile( sint32 index, void **ppBuffer )
{
   _ASSERTE( index >= 0 && index < m_pHeader->m_FileCount );

   // get the size of the full file
   sint32 fileSize = m_Database.GetFileSize( m_pHeader->m_pFileList[ index ] );

   // read it into our scratch file buffer
   m_Database.GetFile( m_pHeader->m_pFileList[ index ], m_pScratchFileBuffer );

   // they won't be interested in the header we attach, so remove it
   *ppBuffer = (void *)new BYTE[ fileSize ];
   
   memcpy( (BYTE *)*ppBuffer, m_pScratchFileBuffer + sizeof( X9ImageHeader ), fileSize );

   return TRUE;
}

BOOL FileExplorer::GetThumbnail( sint32 index, void **ppBuffer, uint32 *pThumbSize )
{
   _ASSERTE( index >= 0 && index < m_pHeader->m_FileCount );

   // get the size of the full file
   sint32 fileSize = m_Database.GetFileSize( m_pHeader->m_pFileList[ index ] );
   fileSize -= sizeof( X9ImageHeader );

   // read it into our scratch file buffer
   m_Database.GetFile( m_pHeader->m_pFileList[ index ], m_pScratchFileBuffer );

   // get the aspect ratio
   X9ImageHeader *pHeader = (X9ImageHeader *)m_pScratchFileBuffer;
   float aspectRatio = (float) pHeader->width / (float) pHeader->height;

   // now have Core convert it to a thumbnail for us.
   g_Core.ConvertImageToThumbnail( (const char *)(m_pScratchFileBuffer + sizeof( X9ImageHeader )), fileSize, aspectRatio, ppBuffer, pThumbSize );

   return TRUE;
}

sint32 FileExplorer::GetFileSize( sint32 index )
{
   _ASSERTE( index >= 0 && index < m_pHeader->m_FileCount );

   // get the size of just the image
   sint32 fileSize = m_Database.GetFileSize( m_pHeader->m_pFileList[ index ] );
   fileSize -= sizeof( X9ImageHeader );

   return fileSize;
}

sint32 FileExplorer::GetFileWidth( sint32 index )
{
   _ASSERTE( index >= 0 && index < m_pHeader->m_FileCount );

   // get the file
   m_Database.GetFile( m_pHeader->m_pFileList[ index ], m_pScratchFileBuffer );

   // return the width out of the header
   return ((X9ImageHeader *)m_pScratchFileBuffer)->width;
}

sint32 FileExplorer::GetFileHeight( sint32 index )
{
   _ASSERTE( index >= 0 && index < m_pHeader->m_FileCount );

   // get the file
   m_Database.GetFile( m_pHeader->m_pFileList[ index ], m_pScratchFileBuffer );

   // return the width out of the header
   return ((X9ImageHeader *)m_pScratchFileBuffer)->height;
}

uint32 FileExplorer::GetFileUniqueID( sint32 index )
{
   _ASSERTE( index >= 0 && index < m_pHeader->m_FileCount );

   // get the file
   m_Database.GetFile( m_pHeader->m_pFileList[ index ], m_pScratchFileBuffer );

   // return the unique ID out of the header
   return ((X9ImageHeader *)m_pScratchFileBuffer)->uniqueID;
}

uint32 FileExplorer::GetNextFileUniqueID( void )
{
   return m_pHeader->m_UniqueID;
}

float FileExplorer::GetFileAspectRatio( sint32 index )
{
   _ASSERTE( index >= 0 && index < m_pHeader->m_FileCount );

   // get the file
   m_Database.GetFile( m_pHeader->m_pFileList[ index ], m_pScratchFileBuffer );

   // return the width out of the header
   X9ImageHeader *pHeader = (X9ImageHeader *)m_pScratchFileBuffer;

   return (float) pHeader->width / (float) pHeader->height;
}

void FileExplorer::GetFileName( sint32 fileIndex, char *pFileName )
{
   m_Database.GetFileName( m_pHeader->m_pFileList[ fileIndex ], pFileName );
}

sint32 FileExplorer::GetFirstFileIndex( void )
{
   return m_pHeader->m_FirstFileIndex;
}

sint32 FileExplorer::GetPrevIndex( sint32 currentIndex )
{
   _ASSERTE( currentIndex >= 0 && currentIndex < m_pHeader->m_FileCount );

   sint32 nextIndex;

   if ( currentIndex == m_pHeader->m_FirstFileIndex )
   {
      nextIndex = -1;
   }
   else
   {
      nextIndex = currentIndex - 1;

      if ( nextIndex < 0 ) nextIndex = m_pHeader->m_FileCount - 1;      
   }

   return nextIndex;
}

sint32 FileExplorer::GetNextIndex( sint32 currentIndex )
{
   _ASSERTE( currentIndex >= 0 && currentIndex < m_pHeader->m_FileCount );

   sint32 nextIndex = currentIndex + 1;

   // is this database wrapped?
   if ( m_pHeader->m_FirstFileIndex )
   {
      // handle our index needing to wrap
      nextIndex %= m_pHeader->m_FileCount;

      // make sure we're not back at the start
      if ( nextIndex == m_pHeader->m_FirstFileIndex )
      {
         // we are, so return an invalid index.
         nextIndex = -1;
      }
   }
   else
   {
      // simply make sure we're not at the end of the linear list.
      if ( nextIndex == m_pHeader->m_FileCount )
      {
         // we are, so return an invalid index.
         nextIndex = -1;
      }
   }

   return nextIndex;
}

sint32 FileExplorer::GetNumFiles( void )
{
   return m_pHeader->m_FileCount;
}

sint32 FileExplorer::GetMaxFiles( void )
{
   return m_pHeader->m_MaxAllowedFiles;
}

sint32 FileExplorer::GetMaxFileSize( void )
{
   return m_pHeader->m_MaxFileSize;
}

uint32 FileExplorer::GetFileIndexByUniqueID( sint32 uniqueID )
{
   uint32 i, size = GetNumFiles( );

   for ( i = 0; i < size; i++ )
   {
      if ( uniqueID == GetFileUniqueID(i) ) break;
   }

   if ( i == size ) return -1;

   return i;
}

void FileExplorer::DumpDatabase( void )
{
   //Debug-Dump out all images from the database
   CreateDirectory( "C:\\X9DatabaseDump", NULL );

   sint32 readIndex = m_pHeader->m_FirstFileIndex;
   sint32 i;
   for ( i = 0; i < m_pHeader->m_FileCount; i++ )
   {
      char fileName[ MAX_PATH ];
      sprintf( fileName, "C:\\X9DatabaseDump\\test%d.jpg", i );

      DumpToFile( fileName, readIndex );

      readIndex = ( readIndex + 1 ) % m_pHeader->m_FileCount;
   }
}

BOOL FileExplorer::DumpToFile( char *pFileName, sint32 index )
{
   FILE *pFile   = NULL;
   char *pBuffer = NULL;
   BOOL success  = FALSE;
   
   do
   {
      GetFile( index, (void **)&pBuffer );
      
      pFile = fopen( pFileName, "wb" );
      if ( !pFile ) break;

      // dump the image to the file
      fwrite( pBuffer, m_Database.GetFileSize( m_pHeader->m_pFileList[ index ] ), 1, pFile );

      success = TRUE;
   }
   while( 0 );

   if ( pBuffer )
   {
      delete [] pBuffer;
   }

   if ( pFile )
   {
      fclose( pFile );
   }

   return success;
}

void FileExplorer::ExecuteCommand( ICommand::Command *pCommand )
{
   const char *pString = pCommand->GetString( );

   if ( 0 == strcmpi(pString, "dump_database") )
   {
      DumpDatabase( );
   }
}

const char * FileExplorer::GetCommandName( void )
{ 
   return "fileExplorer";
}
