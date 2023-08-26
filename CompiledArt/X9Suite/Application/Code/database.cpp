
#include "precompiled.h"
#include "database.h"
#include "securityAttribs.h"
#include "utilityClock.h"
#include "main.h"

Database::Database( void )
{
   m_pFirstFileSectorCache = NULL;
   m_hDatabaseFile         = INVALID_HANDLE_VALUE;
   m_CurrentSector         = 0;
   m_NeedsFlush            = false;
   m_DatabaseCorrupt       = FALSE;

   memset( &m_Header, 0, sizeof( m_Header ) );
}

Database::~Database( )
{
}

void Database::CloseDatabase( void )
{
   if ( m_hDatabaseFile != INVALID_HANDLE_VALUE )
   {
      delete [] m_Header.m_pFreeSectors;

      delete [] m_Header.m_pStartSectors;
      m_Header.m_pStartSectors = NULL;

      delete [] m_pFirstFileSectorCache;
      m_pFirstFileSectorCache = NULL;

      CloseHandle( m_hDatabaseFile );
      m_hDatabaseFile = INVALID_HANDLE_VALUE;

      m_DatabaseCorrupt = FALSE;
   }
}

int Database::GetNumFiles( void )
{
   return m_Header.m_NumFiles;
}

int Database::GetMaxNumFiles( void )
{
   return m_Header.m_MaxFiles;
}

int Database::GetMaxFileSize( void )
{
   return m_Header.m_AverageSectorsPerFile * SECTOR_SIZE;
}

void Database::CreateDatabaseFile( const char *pFileName, int maxFiles, int averageSectorsPerFile, float *pProgressPercent )
{
   ACL *pDacl = NULL;
   
   do
   {
      SECURITY_DESCRIPTOR securityDescriptor;
      if ( !SecurityAttribs::CreateSecurityDescriptor( &securityDescriptor, &pDacl, TEXT("EVERYONE"), GENERIC_READ | GENERIC_WRITE ) ) break;

      SECURITY_ATTRIBUTES securityAttributes;
      securityAttributes.nLength              = sizeof( securityAttributes );
      securityAttributes.bInheritHandle       = FALSE;
      securityAttributes.lpSecurityDescriptor = &securityDescriptor;

      m_hDatabaseFile = CreateFile( pFileName, GENERIC_WRITE, 0, &securityAttributes, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );

      if ( INVALID_HANDLE_VALUE == m_hDatabaseFile ) break;

      m_Header.m_MaxFiles               = maxFiles;
      m_Header.m_AverageSectorsPerFile  = averageSectorsPerFile;
      m_Header.m_MaxSectors             = m_Header.m_MaxFiles * m_Header.m_AverageSectorsPerFile;
      m_Header.m_FreeSectorSize         = m_Header.m_MaxSectors / 8 + 1; // determine the size of our free sectors table
      m_Header.m_RealHeaderSize         = sizeof( m_Header ) + (sizeof( int ) * m_Header.m_MaxFiles) + m_Header.m_FreeSectorSize;
      m_Header.m_NumFiles = 0;

      
      

      // lets calculate the total operations so we can provide percentage feedback
      // (it's the number of dummy files + sectors that need writing)
      float totalOperations     = (float) m_Header.m_MaxFiles + m_Header.m_MaxSectors;
      float operationsCompleted = 0;

      // write the database to the file we opened, 
      DWORD bytesWritten;
      WriteFile( m_hDatabaseFile, &m_Header, sizeof( m_Header ), &bytesWritten, NULL );

      int *pStartSectors = new int[ m_Header.m_MaxFiles ];

      // set the start sector table entries to end of chain and write them
      int i;
      for ( i = 0; i < m_Header.m_MaxFiles; i++ )
      {
         pStartSectors[ i ] = SECTOR_END_OF_CHAIN;
      }
         
      WriteFile( m_hDatabaseFile, pStartSectors, sizeof( int ) * m_Header.m_MaxFiles, &bytesWritten, NULL );

      delete [] pStartSectors;

      // update our progress
      if ( pProgressPercent )
      {
         operationsCompleted += m_Header.m_MaxFiles;
         *pProgressPercent = operationsCompleted / totalOperations;
      }


      // allocate and write out the free sectors list
      m_Header.m_pFreeSectors = (BYTE *) malloc( m_Header.m_FreeSectorSize );

      memset( m_Header.m_pFreeSectors, 0xFF, m_Header.m_FreeSectorSize );

      WriteFile( m_hDatabaseFile, m_Header.m_pFreeSectors, m_Header.m_FreeSectorSize, &bytesWritten, NULL );


      // and create all our sectors.
      Sector sector;
      sector.nextSector = SECTOR_EMPTY;

      for ( i = 0; i < m_Header.m_MaxSectors; i++ )
      {
         WriteFile( m_hDatabaseFile, &sector, sizeof( Sector ), &bytesWritten, NULL );

         // update our progress
         if ( pProgressPercent )
         {
            operationsCompleted += 1.00f;
            *pProgressPercent = operationsCompleted / totalOperations;
         }
      }

      // flush after a write because we're open for read/write
      FlushFileBuffers( m_hDatabaseFile );

      if ( pProgressPercent )
      {
         *pProgressPercent = 1.00f;
      }
   }
   while( 0 );

   if ( pDacl )
   {
      LocalFree( pDacl );
   }

   if ( m_hDatabaseFile != INVALID_HANDLE_VALUE )
   {
      CloseHandle( m_hDatabaseFile );
      m_hDatabaseFile = INVALID_HANDLE_VALUE;
   }
}

BOOL Database::DoesDatabaseFileExist( const char *pFileName )
{
   HANDLE fileHandle = CreateFile( pFileName, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
   
   BOOL doesFileExist = fileHandle != INVALID_HANDLE_VALUE ? TRUE : FALSE;

   if ( fileHandle != INVALID_HANDLE_VALUE )
   {
      CloseHandle( fileHandle );
   }

   return doesFileExist;
}

BOOL Database::OpenDatabase( const char *pFileName )
{
   CloseDatabase( );

   m_hDatabaseFile = CreateFile( pFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

   // read the database in if we opened it
   if ( m_hDatabaseFile != INVALID_HANDLE_VALUE )
   {
      DWORD bytesRead;
      ReadFile( m_hDatabaseFile, &m_Header, sizeof( m_Header ), &bytesRead, NULL );

      // set the start sector table entries to end of chain
      m_Header.m_pStartSectors = new int[ m_Header.m_MaxFiles ];

      // now allocate and read in the sectors
      int i;
      for ( i = 0; i < m_Header.m_MaxFiles; i++ )
      {
         DWORD bytesRead;
         ReadFile( m_hDatabaseFile, &m_Header.m_pStartSectors[ i ], sizeof( int ), &bytesRead, NULL );
      }


      // read in the free sector list
      m_Header.m_pFreeSectors = (BYTE *) malloc( m_Header.m_FreeSectorSize );

      ReadFile( m_hDatabaseFile, m_Header.m_pFreeSectors, m_Header.m_FreeSectorSize, &bytesRead, NULL );


      // allocate a cache we can store all the start sectors in
      m_pFirstFileSectorCache = new Sector[ m_Header.m_MaxFiles ];

      // declare an empty sector so that if there's no file we can store this in the cache.
      Sector emptySector = { 0 };
      emptySector.nextSector = SECTOR_EMPTY;

      // now read in the first sector of each file
      for ( i = 0; i < m_Header.m_MaxFiles; i++ )
      {
         if ( m_Header.m_pStartSectors[ i ] != SECTOR_END_OF_CHAIN )
         {
            JumpToSector( m_Header.m_pStartSectors[ i ] );

            ReadSector( &m_pFirstFileSectorCache[ i ] );
         }
         else
         {
            m_pFirstFileSectorCache[ i ] = emptySector;
         }
      }

      // reset the file pointer
      SetFilePointer( m_hDatabaseFile, m_Header.m_RealHeaderSize, 0, FILE_BEGIN );
      
      m_CurrentSector = 0;
   }

/*
   int i;

   //code to verify the free list (one sector per bit) is
   //working
   for ( i = 0; i < m_Header.m_MaxSectors / 8; i++ )
   {
      //check in blocks of 8 bits because
      //each 8 sectors are packed into a byte
      int value[] = 
      {
         rand( ) & 0x1,
         rand( ) & 0x1,
         rand( ) & 0x1,
         rand( ) & 0x1,
         rand( ) & 0x1,
         rand( ) & 0x1,
         rand( ) & 0x1,
         rand( ) & 0x1,
      };

      int c;

      for ( c = 0; c < 8; c++ )
      {
         UpdateFreeSector( i + c, value[c] );
      }
      
      for ( c = 0; c < 8; c++ )
      {
         bool free = IsSectorFree( i + c );

         if ( free != (0 != value[c]) )
         {
            int checkFailed = 0;
         }
      }

   }
*/
   // and return whether we were able to or not
   return m_hDatabaseFile != INVALID_HANDLE_VALUE ? TRUE : FALSE;
}

int Database::FindFreeStartSector( void )
{
   int i;
   for ( i = 0; i < m_Header.m_MaxFiles; i++ )
   {
      if ( SECTOR_END_OF_CHAIN == m_Header.m_pStartSectors[ i ] )
      {
         return i;
      }
   }

   return -1;
}

void Database::ReadSectors( Sector *pSector, int count )
{
   DWORD bytesRead;

   if ( true == m_NeedsFlush )
   {
      FlushFileBuffers( m_hDatabaseFile );     
      m_NeedsFlush = false;
   }

   if ( m_CurrentSector + count >= m_Header.m_MaxSectors )
   {
      int amountBeforeEnd = m_Header.m_MaxSectors - m_CurrentSector;
      int amountAfterEnd  = count - amountBeforeEnd;

      ReadFile( m_hDatabaseFile, pSector, sizeof( Sector ) * amountBeforeEnd, &bytesRead, NULL );      
      
      SetFilePointer( m_hDatabaseFile, m_Header.m_RealHeaderSize, 0, FILE_BEGIN );

      if ( amountAfterEnd )
      {
         ReadFile( m_hDatabaseFile, pSector, sizeof( Sector ) * amountAfterEnd, &bytesRead, NULL );      
      }

      m_CurrentSector = amountAfterEnd;
   }
   else
   {
      ReadFile( m_hDatabaseFile, pSector, sizeof( Sector ) * count, &bytesRead, NULL );      

      m_CurrentSector += count;
   }
}

void Database::ReadSector( Sector *pSector )
{
   ReadSectors( pSector, 1 );
   /*
   DWORD bytesRead;

   if ( true == m_NeedsFlush )
   {
      FlushFileBuffers( m_hDatabaseFile );     
      m_NeedsFlush = false;
   }

   ReadFile( m_hDatabaseFile, pSector, sizeof( Sector ), &bytesRead, NULL );

   m_CurrentSector++;

   // if we reach the end jump back to the start
   if ( m_Header.m_MaxSectors == m_CurrentSector )
   {
      SetFilePointer( m_hDatabaseFile, m_Header.m_RealHeaderSize, 0, FILE_BEGIN );
      m_CurrentSector = 0;
   }
*/
}

void Database::WriteSector( Sector *pSector )
{
   DWORD bytesWritten;
   if ( !WriteFile( m_hDatabaseFile, pSector, sizeof( Sector ), &bytesWritten, NULL ) )
   {
      DWORD error = GetLastError( );
      _ASSERTE( 0 );
   }
   
   UpdateFreeSector( m_CurrentSector, (pSector->nextSector == SECTOR_EMPTY) );

   m_CurrentSector++;

   // if we reach the end jump back to the start
   if ( m_Header.m_MaxSectors == m_CurrentSector )
   {
      SetFilePointer( m_hDatabaseFile, m_Header.m_RealHeaderSize, 0, FILE_BEGIN );
      m_CurrentSector = 0;
   }

   m_NeedsFlush = true;
}

void Database::JumpToSector( int sectorIndex )
{
   // if the index to seek to is less than our current, we need to start over
   if ( sectorIndex < m_CurrentSector )
   {
      SetFilePointer( m_hDatabaseFile, m_Header.m_RealHeaderSize + (sizeof( Sector ) * sectorIndex), 0, FILE_BEGIN );
   }
   else if ( sectorIndex > m_CurrentSector )
   {
      // otherwise, jump forward from where we're at
      int sectorsToJump = sectorIndex - m_CurrentSector;

      SetFilePointer( m_hDatabaseFile, sizeof( Sector ) * sectorsToJump, 0, FILE_CURRENT );
   }

   m_CurrentSector = sectorIndex;
}

void Database::WriteHeader( void )
{
   m_CurrentSector = 0;
   SetFilePointer( m_hDatabaseFile, 0, 0, FILE_BEGIN );

   DWORD bytesWritten;
   WriteFile( m_hDatabaseFile, &m_Header, sizeof( m_Header ), &bytesWritten, NULL );
   
   int i;
   for ( i = 0; i < m_Header.m_MaxFiles; i++ )
   {
      WriteFile( m_hDatabaseFile, &m_Header.m_pStartSectors[ i ], sizeof( int ), &bytesWritten, NULL );
   }

   // write out the free sectors list
   WriteFile( m_hDatabaseFile, m_Header.m_pFreeSectors, m_Header.m_FreeSectorSize, &bytesWritten, NULL );
}

int Database::GetRequiredSectorCount( int fileSize )
{
   float requiredSectors = ((float)fileSize / (float)SECTOR_SIZE);

   requiredSectors++; //Always add one for the header

   // now get the remainder, and if there is one, we need to round up for one more sector
   float requiredSectorsRemainder = requiredSectors - floor( requiredSectors );
   if ( requiredSectorsRemainder )
   {
      requiredSectors++;
   }

   // add two because fractions will round down, and we need a second for the header.
   return (int)requiredSectors;
}

BOOL Database::FindFreeSectors( int requiredSectors, int *pSectorList )
{
   int i, numSectorsFound = 0;

   for ( i = 0; i < m_Header.m_MaxSectors; i++ )
   {
      if ( IsSectorFree(i) )
      {
         pSectorList[ numSectorsFound ] = i;
         numSectorsFound++;

         // once we find all required sectors, we're done
         if ( numSectorsFound == requiredSectors )
         {
            return TRUE;
         }
      }
   }
   
   // if we get to this point, there simply weren't enough free sectors
   return FALSE;
}

BOOL Database::DeleteFile( const char *pFileName, BOOL assertOnFail )
{
   //Clock clock;
   //clock.Start( );

   // get the file index for this file
   int fileIndex = GetFilesTableIndex( pFileName );

   if ( fileIndex < 0 )
   {
      if ( assertOnFail )
      {
         _ASSERTE( fileIndex > -1 );
      }

      return FALSE;
   }

   //_RPT1( _CRT_WARN, "\t\t1: %.4f\n", clock.MarkSample( ) );

   // read its first sector from cache, and determine how long the file is
   Sector workingSector = m_pFirstFileSectorCache[ fileIndex ];

   int numSectorsInHeader;
   int *pSectorList;
   int fileSize;
   ReadFirstFileSector( &workingSector, NULL, &fileSize, &pSectorList, &numSectorsInHeader );

   //_RPT1( _CRT_WARN, "\t\t2: %.4f\n", clock.MarkSample( ) );

   // get the total sector count
   int totalSectors = GetRequiredSectorCount( fileSize );

   // allocate a local array that we'll use to store the entire sector table
   int *pSectorTable = new int[ totalSectors + 1 ]; // do one extra so we have room for the end of chain index.

   // now copy over what was stored in the first sector
   memcpy( pSectorTable, pSectorList, numSectorsInHeader * sizeof( int ) );

   //_RPT1( _CRT_WARN, "\t\t3: %.4f\n", clock.MarkSample( ) );

   // if we didn't get all the sectors, we'll have to do some reading to finish
   if ( numSectorsInHeader < totalSectors )
   {
      int sectorIndexInTable = numSectorsInHeader - 1;

      do
      {
         // jump to the sector
         JumpToSector( pSectorTable[ sectorIndexInTable ] );

         // read it
         ReadSector( &workingSector );

         // copy its next index into our table
         sectorIndexInTable++;
         pSectorTable[ sectorIndexInTable ] = workingSector.nextSector;
      }
      while( pSectorTable[ sectorIndexInTable ] != SECTOR_END_OF_CHAIN );

      //_RPT1( _CRT_WARN, "\t\t4: %.4f\n", clock.MarkSample( ) );

      // flush after our reading because we're open for read/write
      //FlushFileBuffers( m_hDatabaseFile );
      //m_NeedsFlush = true;

      //_RPT1( _CRT_WARN, "\t\t5: %.4f\n", clock.MarkSample( ) );
   }

   // now write all those sectors back, flagged as blank
   int currentSector = pSectorTable[ 0 ];
   workingSector.nextSector = SECTOR_EMPTY;
   
   int i;
   for ( i = 0; i < totalSectors; i++ )
   {
      JumpToSector( pSectorTable[ i ] );
      // now write it out as blank
      WriteSector( &workingSector );
   }

   //_RPT1( _CRT_WARN, "\t\t6: %.4f\n", clock.MarkSample( ) );


   // clean up our cache
   delete [] pSectorTable;
   delete [] pSectorList;

   // free up the index in the start sector table
   m_Header.m_pStartSectors[ fileIndex ] = SECTOR_END_OF_CHAIN;

   // free up the cache slot
   m_pFirstFileSectorCache[ fileIndex ] = workingSector;

   // decrement the total files in the database
   m_Header.m_NumFiles--;

   WriteHeader( );

   //_RPT1( _CRT_WARN, "\t\t7: %.4f\n", clock.MarkSample( ) );

   // flush after a write because we're open for read/write
   //FlushFileBuffers( m_hDatabaseFile );
   //m_NeedsFlush = true;

   //_RPT1( _CRT_WARN, "\t\t8: %.4f\n", clock.MarkSample( ) );

   // and the file is deleted
   return TRUE;
}


BOOL Database::SetupFirstFileSector( Sector *pSector, const char *pFileName, int fileSize, int *pSectorList )
{
   // get a pointer to the sector's data buffer
   BYTE *pDataPos = pSector->data;

   // copy the name of the file
   int nameLen = (int) strlen( pFileName ) + 1;

   // vaidate the name's length
   if ( nameLen >= MAX_NAME_LENGTH )
   {
      return FALSE;
   }

   memcpy( pDataPos, pFileName, nameLen );

   pDataPos += nameLen;

   // the length of the file will come after the name including its null terminator
   // ex: Image1.bmp0256 would be Image1.bmp, 256 bytes.
   *((int *)pDataPos) = fileSize;

   pDataPos += sizeof( int );

   // copy in as much of the sector list as possible, compressed.
   // the compression is type runtime length, meaning we'll just abbreviate any
   // contiguous sectors.

   // If a file uses sectors 0, 5 - 10, 12, we can abbreviate that to be
   // 05L1012, which would use 5 ints, rather than 8 ints. For a savings, in this case, of 12 bytes.
   int numSectors = GetRequiredSectorCount( fileSize );

   // Before we get all fancy with compression, let's just get the normal sector table working
   // copy as much of the sector list as possible into the table
   int numSectorsToCopy = (SECTOR_SIZE - nameLen - 4) / 4;
   memcpy( pDataPos, pSectorList, min( numSectors, numSectorsToCopy ) * sizeof( int ) );

   // let's build our compressed table. Start by allocating an array to hold potentially ALL sector indices
   //int *pCompressedSectorList = new int[ numSectors ];

   //// start at 1 because we don't need to write the starting sector's index
   //// and end one shy because we'll write that one manually
   //int i; 
   //for ( i = 1; i < numSectors - 1; i++ )
   //{
   //   int numContiguousSectors = 0;

   //   // first test to see if we find any contiguous sectors.
   //   // is our current sector index one less than the next sector index?
   //   int nextSector = i + 1;
   //   while( pSectorList[ i ] == pSectorList[ nextSector ] - 1 )
   //   {
   //      numContiguousSectors++;
   //   }

   //   // write the current sector
   //   pCompressedSectorList[ i ] = pSectorList[ i ];

   //   // if we found some contiguous sectors, write that
   //   if ( numContiguousSectors )
   //   {
   //      char compressionFlag = 'L';
   //      pCompressedSectorList[ i + 1 ] = *((int *)&compressionFlag);
   //      pCompressedSectorList[ i + 2 ] = numContiguousSectors;
   //   }
   //}

   return TRUE;
}

void Database::ReadFirstFileSector( Sector *pSector, char *pFileName, int *pFileSize, int **ppSectorList, int *pNumSectorsInList )
{
   // get a pointer to the sector's data buffer
   BYTE *pDataPos = pSector->data;

   // we're safe to pretend this is a string because we do null terminate
   int nameLen = (int) strlen( (const char *)pDataPos ) + 1;

   // copy the filename out (optional)
   if ( pFileName )
   {
      strcpy( pFileName, (const char *)pDataPos );
   }

   // moving on, read out the file size (optional)
   pDataPos += nameLen;

   int fileSize = *(int *)pDataPos;
   if ( pFileSize )
   {
      *pFileSize = fileSize;
   }

   // moving on, read out the sector list. (optional) 
   pDataPos += sizeof( int );

   // determine how many we were actually able to fit
   int numSectors       = GetRequiredSectorCount( fileSize );
   int numSectorsToCopy = min( numSectors, (SECTOR_SIZE - nameLen - 4) / 4 );

   if ( ppSectorList )
   {
      //Remember it's only as much of the sector list as we could fit!
      *ppSectorList = new int[ numSectorsToCopy ];

      // copy whichever is LESS
      memcpy( *ppSectorList, pDataPos, numSectorsToCopy * sizeof( int ) );
   }

   // if they ask for it, tell them whether
   // what they're getting is th entire sector table or not
   if ( pNumSectorsInList )
   {
      *pNumSectorsInList = numSectorsToCopy;
   }
}

int Database::AddFile( BYTE *pBuffer, const char *pFileName, int fileSize, int *pSectorList )
{
   // update the database with the info for this new file
   if ( m_Header.m_NumFiles >= m_Header.m_MaxFiles )
   {
      return -1;
   }

   m_Header.m_NumFiles++;

   // find a place for us to store the file's first index in our start sector table
   int startSector = FindFreeStartSector( );
   if ( -1 == startSector )
   {
      return -1;
   }
   
   m_Header.m_pStartSectors[ startSector ] = pSectorList[ 0 ];

   int currSector = 0;
   int bytesLeft  = fileSize;

   // before copying the file, setup the first sector with info about the file
   Sector workingSector;
   if ( !SetupFirstFileSector( &workingSector, pFileName, fileSize, pSectorList ) )
   {
      return -1;
   }

   workingSector.nextSector = pSectorList[ currSector + 1 ];

   // write it
   JumpToSector( pSectorList[ currSector ] );

   //DEBUGGING - Read the sector we're about to write and make sure it's really free.
   Sector tempSector;
   ReadSector( &tempSector );
   JumpToSector( pSectorList[ currSector ] );
   if ( tempSector.nextSector != SECTOR_EMPTY )
   {
      _ASSERTE( 0 );
   }
   //

   WriteSector( &workingSector );

   // copy this to our cache
   m_pFirstFileSectorCache[ startSector ] = workingSector;
   
   
   // advance to the next sector and begin the actual file copy
   currSector++;
   
   // use a do/while so that if the file has 0 bytes, we will still correctly allocate it the minimum of 2 sectors. (1 for the filename/size, 1 for the non-existent data)
   do
   {
      int bytesToCopy = min( bytesLeft, SECTOR_SIZE );

      // copy one sector's worth, or the remaining data
      memcpy( workingSector.data, pBuffer, bytesToCopy );

      pBuffer += bytesToCopy;

      bytesLeft -= bytesToCopy;

      // more to copy? setup the next sector
      if ( bytesLeft )
      {
         workingSector.nextSector = pSectorList[ currSector + 1 ];
      }
      else
      {
         // otherwise set the last sector we used to the end of the chain
         workingSector.nextSector = SECTOR_END_OF_CHAIN;
      }

      // write the sector
      JumpToSector( pSectorList[ currSector ] );
      WriteSector( &workingSector );


      currSector++;
   }
   while( bytesLeft );


   WriteHeader( );

   // flush after a write because we're open for read/write
   //FlushFileBuffers( m_hDatabaseFile );
   //m_NeedsFlush = true;

   //return the starting sector for this file
   return startSector;
}

int Database::GetFileSize( int fileIndex )
{
   int fileSize = 0;

   // process the first sector, and that's all we need for the file size
   ReadFirstFileSector( &m_pFirstFileSectorCache[ fileIndex ], NULL, &fileSize, NULL, NULL );

   return fileSize;
}

int Database::GetFilesTableIndex( const char *pFileName )
{
   // returns the index of the file within the m_pStartSectors and
   // m_pFirstFileSector cache.
   int nameLen = (int) strlen( pFileName );

   // we know we didnt' find it if it's too long
   if ( nameLen >= MAX_NAME_LENGTH )
   {
      return -1;
   }

   // go thru all the files in the database until we find a match
   int i;
   for ( i = 0; i < m_Header.m_MaxFiles; i++ )
   {
      // see if there's a valid start sector in the file cache
      if ( m_Header.m_pStartSectors[ i ] != SECTOR_EMPTY )
      {
         // there is, so see if this is the file we want.
         if ( !memcmp( pFileName, m_pFirstFileSectorCache[ i ].data, nameLen ) )
         {
            // we have a match, so reference the start sector table to know which index this sector is.
            // (The cache and sector table are 1 to 1)
            return i;
         }
      }
   }

   // if we're all the way down here, we didn't find the file
   return -1;
}

int Database::GetFilesFirstSectorIndex( const char *pFileName )
{
   int nameLen = (int) strlen( pFileName );

   // we know we didnt' find it if it's too long
   if ( nameLen >= MAX_NAME_LENGTH )
   {
      return -1;
   }

   // go thru all the files in the database until we find a match
   int i;
   for ( i = 0; i < m_Header.m_MaxFiles; i++ )
   {
      // see if there's a valid start sector in the file cache
      if ( m_Header.m_pStartSectors[ i ] != SECTOR_EMPTY )
      {
         // there is, so see if this is the file we want.
         if ( !memcmp( pFileName, m_pFirstFileSectorCache[ i ].data, nameLen ) )
         {
            // we have a match, so reference the start sector table to know which index this sector is.
            // (The cache and sector table are 1 to 1)
            return m_Header.m_pStartSectors[ i ];
         }
      }
   }

   // if we're all the way down here, we didn't find the file
   return -1;
}

int Database::GetFile( const char *pFileName, void *pBuffer )
{
   // get the file index for this file
   int fileIndex = GetFilesFirstSectorIndex( pFileName );

   return GetFile( fileIndex, pBuffer, NULL );
}

int Database::GetFile( int fileIndex, void *pBuffer )
{
   return GetFile( fileIndex, pBuffer, NULL );
}

int Database::EnumerateFiles( int currentIndex )
{
   // given the index they pass in, provide the index of the next file in that order.
   // so if they pass in 0, give them the index of the first file we find in the start sectors table.
   while( SECTOR_END_OF_CHAIN == m_Header.m_pStartSectors[ currentIndex ] )
   {
      currentIndex++;
   }

   // return the index of the next file
   return currentIndex;
}

int Database::GetFile( int fileIndex, void *pBuffer, char *pFileName )
{
   // if we get -1 as a file index, do not process
   if ( -1 == fileIndex )
   {
      return -1;
   }

   int fileSize = GetFileSize( fileIndex );

   // go thru the entire chain and copy the file
   int currentSector = m_Header.m_pStartSectors[ fileIndex ];


   // grab the first sector from cache, it's the file info
   Sector workingSector = m_pFirstFileSectorCache[ fileIndex ];

   // if a valid filename pointer was provided, fill it with the file name
   if ( pFileName )
   {
      strcpy( pFileName, (const char *)workingSector.data );
   }

   // alternatively, if they didn't provide a data buffer, they might have
   // just wanted the file name
   if ( pBuffer )
   {
      BYTE *pCurrPos = (BYTE *)pBuffer;

      currentSector = workingSector.nextSector;

      int bytesLeft = fileSize;
      while( currentSector != SECTOR_END_OF_CHAIN )
      {
         JumpToSector( currentSector );
         ReadSector( &workingSector );

         int bytesToCopy = min( SECTOR_SIZE, bytesLeft );

         memcpy( pCurrPos, workingSector.data, bytesToCopy );

         // jump to the next sector
         currentSector = workingSector.nextSector;

         pCurrPos  += bytesToCopy;
         bytesLeft -= bytesToCopy;

         // as a precaution, test for a bad current sector.
         // there's a bug somehow causing blank sectors to be written
         // in the middle of a file stream.
         if ( -1 == currentSector )
         {
            fileSize = -1;
            m_DatabaseCorrupt = TRUE; //flag that we're corrupt so we can recreate
            break;
         }
      }
   }

   return fileSize;
}

int Database::GetFileName( int fileIndex, char *pFileName )
{
   return GetFile( fileIndex, NULL, pFileName );
}

BOOL Database::IsCorrupt( void )
{
   return m_DatabaseCorrupt;
}

BOOL Database::GenerateDebugMap( const char *pFileName )
{
   // this function will basically draw a map showing each file and its fragmentation
   BOOL success    = FALSE;
   FILE *pFile     = NULL;
   int *pMapBuffer = NULL;

   int mapWidth  = m_Header.m_MaxSectors / 22;
   int mapHeight = m_Header.m_MaxSectors / 22;

   int red       = 64;
   int blue      = 64;
   int green     = 64;
   int colorStep = 0x40 / m_Header.m_NumFiles; // this is how much each file should change in color

   do
   {
      pFile = fopen( pFileName, "wb" );
      if ( !pFile ) break;

      // create a buffer we can use to paint our map to
      pMapBuffer = new int[ m_Header.m_MaxSectors ];
      memset( pMapBuffer, 0, m_Header.m_MaxSectors * 4 );

      int currFile       = 0;
      int filesProcessed = 0;

      // we want to process all files in the database, but they aren't linear in the StartSectors table
      while( filesProcessed != m_Header.m_NumFiles )
      {
         // find the next file in the start sector table
         while( SECTOR_END_OF_CHAIN == m_Header.m_pStartSectors[ currFile ] )
         {
            currFile++;
         }

         // now process all its sectors
         int currSector = m_Header.m_pStartSectors[ currFile ];

         while( currSector != SECTOR_END_OF_CHAIN )
         {
            int pixelColor = 0xFF << 24 | red << 16 | green << 8 | blue;

            // color in the pixel at this sector location
            int xPos = currSector % mapWidth;
            int yPos = currSector / mapWidth;

            pMapBuffer[ (yPos * mapWidth) + xPos ] = pixelColor;

            JumpToSector( currSector );

            Sector workingSector;
            ReadSector( &workingSector );

            currSector = workingSector.nextSector;
         }

         // we've processed one more in our list
         filesProcessed++;

         // and we're done with this file, so move to the next
         currFile++;

         // increment our color
         red   += colorStep;
         blue  += colorStep;
         green += colorStep;
      }

      // now let's create that bitmap.
      BITMAPINFO bmpInfo = { 0 };
      bmpInfo.bmiHeader.biBitCount  = 32;
      bmpInfo.bmiHeader.biHeight    = -mapHeight; //flip it rightside up
      bmpInfo.bmiHeader.biWidth     = mapWidth;
      bmpInfo.bmiHeader.biPlanes    = 1;
      bmpInfo.bmiHeader.biSize      = sizeof( bmpInfo.bmiHeader );
      bmpInfo.bmiHeader.biSizeImage = mapWidth * (bmpInfo.bmiHeader.biBitCount / 8) * mapHeight;

      // setup the bitmap's file header
      BITMAPFILEHEADER bmpFileHeader = { 0 };
      bmpFileHeader.bfType    = 0x4D42;
      bmpFileHeader.bfSize    = sizeof( bmpFileHeader ) + sizeof( bmpInfo.bmiHeader ) + bmpInfo.bmiHeader.biSizeImage;
      bmpFileHeader.bfOffBits = sizeof( bmpFileHeader ) + sizeof( bmpInfo.bmiHeader );

      fwrite( &bmpFileHeader    , sizeof( bmpFileHeader     )  , 1, pFile );
      fwrite( &bmpInfo.bmiHeader, sizeof( bmpInfo.bmiHeader )  , 1, pFile );
      fwrite( pMapBuffer        , bmpInfo.bmiHeader.biSizeImage, 1, pFile );

      success = TRUE;
   }
   while( 0 );

   delete [] pMapBuffer;

   if ( pFile )
   {
      fclose( pFile );
   }

   return success;
}

void Database::UpdateFreeSector(
   int sector,
   int free
)
{
   BYTE *pSlot = m_Header.m_pFreeSectors + (sector / 8);
   *pSlot = (*pSlot & ~(1 << (sector % 8))) | (BYTE) (free << (sector % 8));
}

BOOL Database::IsSectorFree(
   int sector
)
{
   BYTE *pSlot = m_Header.m_pFreeSectors + (sector / 8);
   return 0 != (*pSlot & (1 << (sector % 8)));
}

