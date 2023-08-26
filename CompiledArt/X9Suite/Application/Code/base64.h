
#ifndef BASE64_H_
#define BASE64_H_

size_t b64size(
   size_t length
);

BOOL b64encode(
   char *pEncoded,
   size_t encodedLength,
   const char *pInBuf, 
   size_t inBufLength
);

BOOL b64decode(
   char *pDecoded,
   size_t decodedLength,
   const char *pInBuf, 
   size_t encodedLength
);

#endif
