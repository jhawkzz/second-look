
#include "precompiled.h"
#include "productKey.h"

int KeyRand::m_Seed;
int KeyRand::m_LastRand;

BOOL ProductKey::VerifyKey(
   Key *pKey,
   int *pOutSeed //= NULL
)
{
   Key key;
   size_t i;

   //key is reversed order, so unreverse now
   for ( i = 0; i < sizeof(pKey->key); i++ )
   {
      key.key[ i ] = pKey->key[ sizeof(pKey->key) - i - 2 ];
   }

   //convert >= A back to the original hex
   for ( i = 0; i < sizeof(key.key); i++ )
   {
      key.key[ i ] = toupper( key.key[ i ] );   

      if ( key.key[ i ] >= 'A' && key.key[ i ] <= 'Z' )
      {
         key.key[ i ] -= (char) (i % 10);
      }
   }

   int seed = 0;

   //now build the seed from the first four hex numbers
   for ( i = 0; i < 4; i++ )
   {
      int number;

      if ( key.key[ i ] >= 'A' && key.key[ i ] <= 'F' )
      {
         number = 10 + (key.key[ i ] - 'A');
      }
      else
      {
         number = key.key[ i ] - '0';
      }
         
      seed |= number << (16 - (i * 4) - 4);
   }

   //auto generate what the key would be based on the seed
   GenerateKey( &key, seed );

   if ( pOutSeed ) *pOutSeed = seed;

   //if the generated key from the seed matches the key passed in
   //then we have a valid key
   return ( 0 == memcmp(&key, pKey, sizeof(key)) );
}

void ProductKey::GenerateKey(
   Key *pKeyOut,
   int startSeed //= 0
)
{
   Key key;
   int seed[ 4 ] = { 0 };
   size_t i;

   if ( 0 == startSeed )
   {
      seed[ 0 ] = abs(KeyRand::Rand( ) % 0xffff);
      
      //zero means no start seed, so make sure our
      //valid start seed is not zero
      if ( 0 == seed[ 0 ] ) seed[ 0 ] = 1;
   }
   else
   {
      //start seed will always be a 16 bit number
      //this just makes sure there are no high bits set
      seed[ 0 ] = 0xffff & startSeed;
   }

   KeyRand::Seed( seed[ 0 ] );
   seed[ 1 ] = abs(KeyRand::Rand( ) % 0xffff);

   KeyRand::Seed( seed[ 1 ] );
   seed[ 2 ] = abs(KeyRand::Rand( ) % 0xffff);

   KeyRand::Seed( seed[ 2 ] );
   seed[ 3 ] = abs(KeyRand::Rand( ) % 0xffff);

   _snprintf( key.key, sizeof(key.key) - 1, "%04hx%04hx%04hx%04hx", seed[ 0 ], seed[ 1 ], seed[ 2 ], seed[ 3 ] );
   
   for ( i = 0; i < sizeof(key.key); i++ )
   {
      key.key[ i ] = toupper( key.key[ i ] );   
      
      if ( key.key[ i ] >= 'A' && key.key[ i ] <= 'F' )
      {
         key.key[ i ] += (char) (i % 10);
      }
   }

   for ( i = 0; i < sizeof(key.key); i++ )
   {
      pKeyOut->key[ i ] = key.key[ sizeof(key.key) - i - 2 ];
   }

   pKeyOut->key[ sizeof(pKeyOut->key) - 1 ] = NULL;
}
