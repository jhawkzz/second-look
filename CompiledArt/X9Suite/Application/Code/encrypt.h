
#ifndef ENCRYPT_H_
#define ENCRYPT_H_

class EncryptResult
{
friend class Encryptor;

private:
   char  *m_pBuffer;
   uint32 m_Size;

public:
   EncryptResult( void )
   {
      m_Size    = 0;
      m_pBuffer = NULL;
   }

   char  *GetBuffer( void ) const { return m_pBuffer; }
   uint32 GetSize( void )   const { return m_Size; }
   
   void Free( void )
   {
      free( m_pBuffer );

      m_Size    = 0;
      m_pBuffer = NULL;
   }
};

class EncryptResultScope
{
private:
   EncryptResult m_Result;

public:

   EncryptResultScope(EncryptResult result)
   {
      m_Result = result;
   }

   ~EncryptResultScope( void )
   {
      m_Result.Free( );
   }

   EncryptResult GetResult( void )
   {
      return m_Result;
   }
};

class Encryptor
{
private:
   static const uint32 FIRST_KEY  = 0x5150;
   static const uint32 SECOND_KEY = 0x1984;

   static const uint32 DEFAULT_VALUE_ONE  = 319;
   static const uint32 DEFAULT_VALUE_TWO  = 79;
   static const uint32 DEFAULT_VALUE_FOUR = 117;

private:
   void  ResetValues( void ); // called before encrypt/decrypt to reset value counts

   int   Value    ( void );
   int   Value2   ( void );
   int   Value4   ( void );
   int   SubTable1( unsigned char *pByte );
   int   SecondKey( void );

public:
   EncryptResult Encrypt( const void *pIn, int inSize );
   EncryptResult Decrypt( const void *pIn, int inSize );

   EncryptResult Encrypt( int value ) { return Encrypt( &value, sizeof(value) ); }

   EncryptResult AllocEncrypted( int decryptedSize )
   {
      EncryptResult result;

      result.m_Size    = decryptedSize * 2;
      result.m_pBuffer = (char *) malloc( result.m_Size );

      return result;
   }

public:
   int   m_Value;
   int   m_Value2;
   int   m_Value4;
   int   m_SecondKey;
};

#endif
