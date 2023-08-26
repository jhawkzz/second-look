
#ifndef DATABASE_H_
#define DATABASE_H_

/*
Database format:
A file is stored in sectors in the database. Each sector is SECTOR_SIZE long, so the file is broken up based on fileSize / SECTOR_SIZE and put into that many sectors.
Each sector contains the index to the next sector used by the file. The file's last sector will have a nextIndex of SECTOR_END_OF_CHAIN, letting us know there are no more sectors.
The first sector for any file is unique, and simply contains the file's name, size, and the index of the next sector (which is where the data actually begins.)

Following is a simple example:
Assume the database is empty, and SECTOR_SIZE is 1024

Name: image.bmp FileSize: 2000 bytes

Sector 0 will contain the file name 'image.bmp' and 2048 so we know how big the file is.
Sector 0's nextSector will be 1

Sector 1 will contain 1024 bytes from the file (the first half of the file)
Sector 1's nextSector will be 2

Sector 2 will contain 976 bytes from the file (the rest of the file)
Sector 2's nextSector will be SECTOR_END_OF_CHAIN


When reading a file, first we take the name of the file and search all the sectors in the m_StartSectors table
until we find a match. 
Next we take its first sector and copy out the name and file length
Finally, we walk all the sectors used by the file and copy the data into a buffer.

Database members:
m_StartSectors - This is a table containing the starting sector for every file in the database. From here, you can index the first sector of a file.
m_Sector - This is the array of all sectors in the database. Remember, this can be completely fragmented and random.
*/

#define SECTOR_SIZE              (1024) //Arbitrary, but should slice the file up nicely
#define SECTOR_EMPTY             (-1)
#define SECTOR_END_OF_CHAIN      (0xEFFFFFFF)
#define MAX_NAME_LENGTH          (MAX_PATH)

typedef struct _Sector
{
   int nextSector; //If available, set to SECTOR_EMPTY
   BYTE data[ SECTOR_SIZE ];
}
Sector;

typedef struct _DatabaseHeader
{
   //these are non-configurable, and set when the database is created
   int m_MaxFiles;
   int m_AverageSectorsPerFile;
   int m_MaxSectors;
   
   //Use this instead of sizeof( m_Header ), because this includes the start sector table size.
   int m_RealHeaderSize; 
   
   int m_NumFiles;

   int *m_pStartSectors; //If available, set to SECTOR_END_OF_CHAIN

   int   m_FreeSectorSize;
   BYTE *m_pFreeSectors;
}
DatabaseHeader;

class Database
{
public:

        Database ( void );
        ~Database( );

   int GetNumFiles   ( void );
   int GetMaxNumFiles( void );
   int GetMaxFileSize( void );
   int EnumerateFiles( int currentIndex );

   BOOL GenerateDebugMap( const char *pFileName );

   BOOL OpenDatabase ( const char *pFileName );
   void CloseDatabase( void );

   BOOL DoesDatabaseFileExist( const char *pFileName );
   void CreateDatabaseFile   ( const char *pFileName, int maxFiles, int averageSectorsPerFile, float *pProgressPercent = NULL );

   int  GetRequiredSectorCount( int fileSize );
   BOOL FindFreeSectors       ( int requiredSectors, int *pSectorList );
   int  AddFile               ( BYTE *pBuffer, const char *pFileName, int fileSize, int *pSectorList );
   BOOL DeleteFile            ( const char *pFileName, BOOL assertOnFail );

   int GetFileSize( int fileIndex );
   int GetFileName( int fileIndex, char *pFileName );

   int GetFile( const char *pFileName, void *pBuffer );
   int GetFile( int fileIndex, void *pBuffer );

   BOOL IsCorrupt( void );

private:

   void ReadSectors ( Sector *pSector, int count );
   void ReadSector  ( Sector *pSector );
   void WriteSector ( Sector *pSector );
   void JumpToSector( int sectorIndex );

   void WriteHeader( void );

   int  FindFreeStartSector ( void );
   BOOL SetupFirstFileSector( Sector *pSector, const char *pFileName, int fileSize, int *pSectorList );
   void ReadFirstFileSector ( Sector *pSector, char *pFileName, int *pFileSize, int **ppSectorList, int *pNumSectorsInList );

   int GetFilesFirstSectorIndex( const char *pFileName );
   int GetFilesTableIndex      ( const char *pFileName );
   int GetFile                 ( int fileIndex, void *pBuffer, char *pFileName );

   void BuildFreeList   ( float *pProgressPercent = NULL );
   void UpdateFreeSector( int sector, int free );
   BOOL IsSectorFree    ( int sector );

private:

   HANDLE         m_hDatabaseFile;
   int            m_CurrentSector;
   DatabaseHeader m_Header;
   bool           m_NeedsFlush;
   BOOL           m_DatabaseCorrupt;
   //This stores a copy of the first sector for each file. This greatly minimizes disk access.
   Sector        *m_pFirstFileSectorCache;
};

#endif
