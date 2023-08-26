
#ifndef MEMORYSTREAM_H_
#define MEMORYSTREAM_H_

class MemoryStream
{
protected:
   char   *m_pData;
   size_t  m_Size;
   size_t  m_MaxSize;
   size_t  m_GrowBy;

public:
   MemoryStream( void )
   {
      m_pData   = NULL;
      m_Size    = 0;
      m_MaxSize = 0;
      m_GrowBy  = 0;
   }

   void Create(
      size_t growBy = 1024
   )
   {
      m_pData   = NULL;
      m_Size    = 0;
      m_MaxSize = 0;
      m_GrowBy  = growBy;
   }

   void Destroy( void )
   {
      free( m_pData );

      m_pData   = NULL;
      m_Size    = 0;
      m_MaxSize = 0;
      m_GrowBy  = 0;
   }

   void Write(
      const void *pData,
      size_t size
   );
   
   void Insert(
      uint32 pos,
      const void *pData,
      size_t size
   );

   void Advance(
      uint32 amount
   );

   void Resize(
      size_t newSize
   );

   void *GetBuffer( void ) { return m_pData; }
   
   size_t GetSize( void ) { return m_Size; }
   
   void Clear( void ) { m_Size = 0; }
};

#endif
