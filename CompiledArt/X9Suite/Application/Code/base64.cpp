#include "precompiled.h"
#include "base64.h"

size_t b64size(
   size_t length
)
{
   size_t div   = length / 3;
   size_t rem	 = length % 3;
   size_t chars = div*4 + rem + 1;

   return div * 4 + rem * 4 + 1;
}

//
// taken from:
// Copyright (c) 2000 Virtual Unlimited B.V.
// Author: Bob Deblier <bob@virtualunlimited.com>
// thanks bob ;)
//
BOOL b64encode(
   char *pEncoded,
   size_t encodedLength,
   const char *pInBuf, 
   size_t inBufLength
)
{
	static const char* to_b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		 
   size_t div   = inBufLength / 3;
   size_t rem	 = inBufLength % 3;
   size_t chars = div*4 + rem + 1;

   if ( encodedLength < div * 4 + rem * 4 + 1 ) return FALSE;

   const char * data = pInBuf;

   char *buf = pEncoded;

   chars = 0;

   while (div > 0)
   {
      buf[0]	= to_b64[ (data[0] >> 2 ) & 0x3f];
      buf[1]	= to_b64[((data[0] << 4 ) & 0x30) + ((data[1] >> 4) & 0xf)];
      buf[2]	= to_b64[((data[1] << 2 ) & 0x3c) + ((data[2] >> 6) & 0x3)];
      buf[3]	= to_b64[  data[2] & 0x3f];
         
      data	+= 3;
      buf   += 4;
      chars	+= 4;

      div--;
   }

   switch( rem )
   {
     case 2:
             buf[0] = to_b64[ (data[0] >> 2) & 0x3f];
             buf[1] = to_b64[((data[0] << 4) & 0x30) + ((data[1] >> 4) & 0xf)];
             buf[2] = to_b64[ (data[1] << 2) & 0x3c];
             buf[3] = '=';
             buf   += 4;
             chars += 4;
             break;
     case 1:
             buf[0] = to_b64[ (data[0] >> 2) & 0x3f];
             buf[1] = to_b64[ (data[0] << 4) & 0x30];
             buf[2] = '=';
             buf[3] = '=';
             buf   += 4;
             chars += 4;
             break;
   }

   *buf = '\0';
   
   return TRUE;
}

static const char base64DecTable[256] = {
                   00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00,
                   00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00,
                   00,00,00,00, 00,00,00,00, 00,00,00,62, 00,00,00,63,
                   52,53,54,55, 56,57,58,59, 60,61,00,00, 00,00,00,00,
                    0, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
                   15,16,17,18, 19,20,21,22, 23,24,25,00, 00,00,00,00,
                   00,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
                   41,42,43,44, 45,46,47,48, 49,50,51,00, 00,00,00,00,
                   00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00,
                   00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00,
                   00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00,
                   00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00,
                   00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00,
                   00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00,
                   00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00,
                   00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00,
                  };



BOOL b64decode(
   char *pDecoded,
   size_t decodedLength,
   const char *pInBuf, 
   size_t encodedLength
)
{

   if ( decodedLength < encodedLength / 4 * 3 + 1 ) return FALSE; 

   size_t t,o;

   t=o=0;

   do {
    pDecoded[o++]=((base64DecTable[(int)pInBuf[t]]   << 2 ) & 0xfc ) |
                ((base64DecTable[(int)pInBuf[t+1]] >> 4 ) & 0x03 );

    pDecoded[o++]=((base64DecTable[(int)pInBuf[t+1]] << 4 ) & 0xf0 ) |
                ((base64DecTable[(int)pInBuf[t+2]] >> 2 ) & 0x0f );

    pDecoded[o++]=((base64DecTable[(int)pInBuf[t+2]] << 6 ) & 0xc0 ) |
                ((base64DecTable[(int)pInBuf[t+3]]));

    t+=4;
   } while(t<encodedLength);
   
   return TRUE;
}
