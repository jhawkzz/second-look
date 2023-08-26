
#ifndef PRODUCTKEY_H_
#define PRODUCTKEY_H_

class KeyRand
{
protected:
   static int m_Seed;
   static int m_LastRand;

public:
   static void Seed( int seed )
   {
      m_Seed = seed;
      m_LastRand = m_Seed;
   };

   static int Rand( void ) 
   { 
      m_LastRand = (m_LastRand + 1) * m_Seed;
      
      return m_LastRand;
   }
};

class ProductKey
{
public:
   struct Key
   {
      char key[ 16 + 1 ];
   };

public:
   static BOOL VerifyKey(
      Key *pKey,
      int *pOutSeed = NULL
   );

   static void GenerateKey(
      Key *pKeyOut,
      int seed = 0
   );
};

#endif //PRODUCTKEY_H_
