
#include "precompiled.h"
#include "memoryStream.h"

void MemoryStream::Write(
   const void *pData,
   size_t size
)
{
   if ( m_Size + size > m_MaxSize )
   {
      Resize( m_Size + size + m_GrowBy );
   }

   memcpy( m_pData + m_Size, pData, size );
   m_Size += size;
}

void MemoryStream::Insert(
   uint32 pos,
   const void *pData,
   size_t size
)
{
   if ( m_Size + size > m_MaxSize )
   {
      Resize( m_Size + size + m_GrowBy );
   }

   memmove( m_pData + size + pos, m_pData + pos, m_Size );
   memcpy( m_pData, pData, size );

   m_Size += size;
}

void MemoryStream::Advance(
   uint32 amount
)
{
   memmove( m_pData, m_pData + amount, m_Size - amount );

   m_Size -= amount;
}

void MemoryStream::Resize(
   size_t newSize
)
{
   m_MaxSize = newSize;
   m_pData = (char *) realloc( m_pData, m_MaxSize );
}