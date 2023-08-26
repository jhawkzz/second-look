
#include "precompiled.h"
#include "encrypt.h"

int Encryptor::SubTable1( unsigned char *pByte )
{
	if ( *pByte == 'a' )
	{
	   *pByte = '6';
	}
	else if ( *pByte == 'b' )
	{
	   *pByte = 'u';
	}
	else if ( *pByte == 'c' )
	{
	   *pByte = 'r';
	}
	else if ( *pByte == 'd' )
	{
	   *pByte = 'y';
	}
	else if ( *pByte == 'e' )
	{
	   *pByte = 'm';
	}
	else if ( *pByte == 'f' )
	{
	   *pByte = '8';
	}
	else if ( *pByte == 'g' )
	{
	   *pByte = 'x';
	}
	else if ( *pByte == 'h' )
	{
	   *pByte = '4';
	}
	else if ( *pByte == 'i' )
	{
	   *pByte = 'w';
	}
	else if ( *pByte == 'j' )
	{
	   *pByte = 'o';
	}
	else if ( *pByte == 'k' )
	{
	   *pByte = '1';
	}
	else if ( *pByte == 'l' )
	{
	   *pByte = 's';
	}
	else if ( *pByte == 'm' )
	{
	   *pByte = 'e';
	}
	else if ( *pByte == 'n' )
	{
	   *pByte = '3';
	}
	else if ( *pByte == 'o' )
	{
	   *pByte = 'j';
	}
	else if ( *pByte == 'p' )
	{
	   *pByte = '0';
	}
	else if ( *pByte == 'q' )
	{
	   *pByte = '5';
	}
	else if ( *pByte == 'r' )
	{
	   *pByte = 'c';
	}
	else if ( *pByte == 's' )
	{
	   *pByte = 'l';
	}
	else if ( *pByte == 't' )
	{
	   *pByte = '2';
	}
	else if ( *pByte == 'u' )
	{
	   *pByte = 'b';
	}
	else if ( *pByte == 'v' )
	{
	   *pByte = '9';
	}
	else if ( *pByte == 'w' )
	{
	   *pByte = 'i';
	}
	else if ( *pByte == 'x' )
	{
	   *pByte = 'g';
	}
	else if ( *pByte == 'y' )
	{
	   *pByte = 'd';
	}
	else if ( *pByte == 'z' )
	{
	   *pByte = '7';
	}
	else if ( *pByte == '1' )
	{
	   *pByte = 'k';
	}
	else if ( *pByte == '2' )
	{
	   *pByte = 't';
	}
	else if ( *pByte == '3' )
	{
	   *pByte = 'n';
	}
	else if ( *pByte == '4' )
	{
	   *pByte = 'h';
	}
	else if ( *pByte == '5' )
	{
	   *pByte = 'q';
	}
	else if ( *pByte == '6' )
	{
	   *pByte = 'a';
	}
	else if ( *pByte == '7' )
	{
	   *pByte = 'z';
	}
	else if ( *pByte == '8' )
	{
	   *pByte = 'f';
	}
	else if ( *pByte == '9' )
	{
	   *pByte = 'v';
	}
	else if ( *pByte == '0' )
	{
		*pByte = 'p';
	}
	else if ( *pByte == 'A' )
	{
		*pByte = 'R';
	}
	else if ( *pByte == 'B' )
	{
		*pByte = 'J';
	}
	else if ( *pByte == 'C' )
	{
		*pByte = 'N';
	}
	else if ( *pByte == 'D' )
	{
		*pByte = 'L';
	}
	else if ( *pByte == 'E' )
	{
		*pByte = 'O';
	}
	else if ( *pByte == 'F' )
	{
		*pByte = 'T';
	}
	else if ( *pByte == 'G' )
	{
		*pByte = 'U';
	}
	else if ( *pByte == 'H' )
	{
		*pByte = 'Q';
	}
	else if ( *pByte == 'I' )
	{
		*pByte = 'S';
	}
	else if ( *pByte == 'J' )
	{
		*pByte = 'B';
	}
	else if ( *pByte == 'K' )
	{
		*pByte = 'V';
	}
	else if ( *pByte == 'L' )
	{
		*pByte = 'D';
	}
	else if ( *pByte == 'M' )
	{
		*pByte = 'W';
	}
	else if ( *pByte == 'N' )
	{
		*pByte = 'C';
	}
	else if ( *pByte == 'O' )
	{
		*pByte = 'E';
	}
	else if ( *pByte == 'P' )
	{
		*pByte = 'Z';
	}
	else if ( *pByte == 'Q' )
	{
		*pByte = 'H';
	}
	else if ( *pByte == 'R' )
	{
		*pByte = 'A';
	}
	else if ( *pByte == 'S' )
	{
		*pByte = 'I';
	}
	else if ( *pByte == 'T' )
	{
		*pByte = 'F';
	}
	else if ( *pByte == 'U' )
	{
		*pByte = 'G';
	}
	else if ( *pByte == 'V' )
	{
		*pByte = 'K';
	}
	else if ( *pByte == 'W' )
	{
		*pByte = 'M';
	}
	else if ( *pByte == 'X' )
	{
		*pByte = 'Y';
	}
	else if ( *pByte == 'Y' )
	{
		*pByte = 'X';
	}
	else if ( *pByte == 'Z' )
	{
		*pByte = 'P';
	}
	else if ( *pByte == '*' )
	{
		*pByte = ':';
	}
	else if ( *pByte == '"' )
	{
		*pByte = ',';
	}
	else if ( *pByte == '#' )
	{
		*pByte = ';';
	}
	else if ( *pByte == '%' )
	{
		*pByte = '_';
	}
	else if ( *pByte == ')' )
	{
		*pByte = '+';
	}
	else if ( *pByte == '(' )
	{
		*pByte = '<';
	}
	else if ( *pByte == '^' )
	{
		*pByte = '$';
	}
	else if ( *pByte == '>' )
	{
		*pByte = '!';
	}
	else if ( *pByte == '{' )
	{
		*pByte = '}';
	}
	else if ( *pByte == '}' )
	{
		*pByte = '{';
	}
	else if ( *pByte == '!' )
	{
		*pByte = '>';
	}
	else if ( *pByte == '$' )
	{
		*pByte = '^';
	}
	else if ( *pByte == '<' )
	{
		*pByte = '(';
	}
	else if ( *pByte == '+' )
	{
		*pByte = ')';
	}
	else if ( *pByte == '_' )
	{
		*pByte = '%';
	}
	else if ( *pByte == ';' )
	{
		*pByte = '#';
	}
	else if ( *pByte == ',' )
	{
		*pByte = '"';
	}
	else if ( *pByte == ':' )
	{
		*pByte = '*';
	}
	
	return 0;
}

void Encryptor::ResetValues( void )
{
   m_Value     = DEFAULT_VALUE_ONE;
   m_Value2    = DEFAULT_VALUE_TWO;
   m_Value4    = DEFAULT_VALUE_FOUR;
   m_SecondKey = SECOND_KEY;
}

int Encryptor::Value( void )
{
	m_Value++;
 
	return m_Value;
}

int Encryptor::Value2( void )
{
	m_Value2++;
 
	return m_Value2;
}

int Encryptor::SecondKey( void )
{
	m_SecondKey++;
 
	return m_SecondKey;
}

//dummy byte
int Encryptor::Value4( void )
{
	m_Value4++;
 
	return m_Value4;
}

EncryptResult Encryptor::Encrypt( const void *pIn, int inSize )
{
   ResetValues( );

	unsigned char byte;
	int myVal = Value();
	int myVal2 = Value2();
	int myVal3 = SecondKey();
	int myVal4 = Value4();
	
	srand( FIRST_KEY );

   EncryptResult result;
   
   result.m_Size    = inSize * 2;
   result.m_pBuffer = (char *) malloc( result.m_Size );

   char *pOut = result.m_pBuffer;

   BYTE *pInCurrPos  = (BYTE *)pIn;
   BYTE *pOutCurrPos = (BYTE *)pOut;
	
	int current = 0;
	
	while ( pInCurrPos  - (BYTE *)pIn  < inSize )
	{
      byte = *pInCurrPos;

		SubTable1( &byte );
		
		byte += myVal = Value();

		byte -= rand();

		byte += myVal2 = Value2();

		byte += myVal3 = SecondKey();

      *pOutCurrPos = byte;
      pOutCurrPos++;

		char dummyByte = 'w';
		dummyByte += myVal4 = Value4();
		
      *pOutCurrPos = dummyByte;
      pOutCurrPos++;

      pInCurrPos++;
	}

   return result;
}

EncryptResult Encryptor::Decrypt( const void *pIn, int inSize )
{
   ResetValues( );

   unsigned char byte;
	int myVal = Value();
	int myVal2 = Value2();
	int myVal3 = SecondKey();
	int myVal4 = Value4();

	srand( FIRST_KEY );

   EncryptResult result;
   
   result.m_Size    = inSize / 2;
   result.m_pBuffer = (char *) malloc( result.m_Size );

   char *pOut = result.m_pBuffer;

   BYTE *pInCurrPos  = (BYTE *)pIn;
   BYTE *pOutCurrPos = (BYTE *)pOut;

	int current = 0;

	while ( pInCurrPos  - (BYTE *)pIn  < inSize )
	{
      // read the first byte
      byte = *pInCurrPos;
      pInCurrPos++;

      // throw away the dummy byte
		char dummyByte = 'w';
		dummyByte -= myVal4 = Value4();
		dummyByte = *pInCurrPos;
      pInCurrPos++;
		
		byte -= myVal3 = SecondKey();
		
		byte -= myVal2 = Value2();

		byte += rand();

		byte -= myVal = Value();

		SubTable1( &byte );

      *pOutCurrPos = byte;
      pOutCurrPos++;
	}

   return result;
}
