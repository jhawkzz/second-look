
#include "precompiled.h"
#include "imageStream.h"

ImageStream::ImageStream( void )
{
   m_RefCount = 0;
   m_pBuffer  = NULL;
   m_pCurrPos = NULL;
   
   memset( &m_StatStg, 0, sizeof( m_StatStg ) );
   
   m_StatStg.grfLocksSupported = LOCK_WRITE;
   m_StatStg.cbSize.QuadPart   = 0;
   m_StatStg.type              = STGTY_STREAM;
}

ImageStream::~ImageStream( )
{
   delete [] m_pBuffer;
}

ULONG STDMETHODCALLTYPE ImageStream::AddRef( void )
{
   m_RefCount++;

   return m_RefCount;
}

ULONG STDMETHODCALLTYPE ImageStream::Release( void )
{
   m_RefCount--;

   _ASSERT( m_RefCount >= 0 );

   return m_RefCount;
}

HRESULT STDMETHODCALLTYPE ImageStream::QueryInterface( REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject )
{
	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE ImageStream::Stat( STATSTG *pstatstg, DWORD grfStatFlag )
{
   memset( pstatstg, 0, sizeof( STATSTG ) );

   pstatstg->cbSize            = m_StatStg.cbSize;
   pstatstg->type              = m_StatStg.type;
   pstatstg->grfLocksSupported = m_StatStg.grfLocksSupported;

   // if it expects us to provide a name, throw an assert for now so we implement it later.
   if ( grfStatFlag != STATFLAG_NONAME )
   {
      _ASSERT( 0 );
   }
      
   return S_OK;
}

HRESULT STDMETHODCALLTYPE ImageStream::SetSize( ULARGE_INTEGER libNewSize )
{
   // expand the stream
   if ( libNewSize.QuadPart > m_StatStg.cbSize.QuadPart )
   {
      // let's make the buffer its current size + what they want
      BYTE *pNewBuffer = new BYTE[ (ULONG) libNewSize.QuadPart ];

      // copy over what's been written (Safe because if m_pBuffer is null then quadPart must be null too)
      memcpy( pNewBuffer, m_pBuffer, (size_t)m_StatStg.cbSize.QuadPart );
      
      // now free the old buffer
      delete [] m_pBuffer;

      // assign the new one
      m_pBuffer  = pNewBuffer;

      // our position pointer is unaffected in this case
   }
   // truncate the stream
   else if ( libNewSize.QuadPart < m_StatStg.cbSize.QuadPart )
   {
      // just lower quad part, no need to reallocate. 
      // When we delete this buffer it'll get cleaned up.

      // bring our pointer in if we need to
      m_pCurrPos = min( m_pCurrPos, m_pBuffer + libNewSize.QuadPart );
   }

   m_StatStg.cbSize.QuadPart = libNewSize.QuadPart;

   return S_OK;
}

HRESULT STDMETHODCALLTYPE ImageStream::Seek( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition )
{
   HRESULT result = S_OK;

   switch( dwOrigin )
   {
      // from the beginning
      case STREAM_SEEK_SET:
      {
         m_pCurrPos = m_pBuffer + dlibMove.QuadPart;
         break;
      }

      // from the end
      case STREAM_SEEK_END:
      {
         m_pCurrPos = m_pBuffer + m_StatStg.cbSize.QuadPart + dlibMove.QuadPart;
         break;
      }

      // from the current position
      case STREAM_SEEK_CUR:
      {
         m_pCurrPos += dlibMove.QuadPart;
         break;
      }

      default:
      {
         result = STG_E_INVALIDFUNCTION;
         break;
      }
   }

   // validate the curr position. According to MSDN, it's ok to go beyond the end of the buffer.
   if ( m_pCurrPos < m_pBuffer || m_pCurrPos > m_pBuffer + m_StatStg.cbSize.QuadPart )
   {
      result = STG_E_INVALIDFUNCTION;
   }

   // if they want the pointer, give it to em
   if ( plibNewPosition )
   {
      plibNewPosition->QuadPart = (ULONGLONG)m_pCurrPos;
   }

   return result;
}

HRESULT STDMETHODCALLTYPE ImageStream::Read( void *pv, ULONG cb, ULONG *pcbRead )
{
   // clamp the amount of bytes they want to what's available
   ULONG bytesToRead = min( cb, (ULONG) ( (m_pBuffer + m_StatStg.cbSize.QuadPart) - m_pCurrPos ) );

   // copy the memory
   memcpy( pv, m_pCurrPos, bytesToRead );

   // advance our pointer
   m_pCurrPos += bytesToRead;

   // if they want it, tell them how many bytes we read
   if ( pcbRead )
   {
      *pcbRead = bytesToRead;
   }

   return S_OK;
}

HRESULT STDMETHODCALLTYPE ImageStream::Write( const void *pv, ULONG cb, ULONG *pcbWritten )
{
   // clamp the amount of bytes they want write to what's available
   ULONG bytesToWrite = min( cb, (ULONG) ( (m_pBuffer + m_StatStg.cbSize.QuadPart) - m_pCurrPos ) );

   // if we've run out of room, we need to re-allocate a larger array. Let's go ahead and do that.
   if ( bytesToWrite < cb )
   {
      // let's make the buffer its current size + what they want
      BYTE *pNewBuffer = new BYTE[ (ULONG) (m_StatStg.cbSize.QuadPart + cb) ];

      // copy over what's been written (Safe because if m_pBuffer is null then quadPart must be null too)
      memcpy( pNewBuffer, m_pBuffer, (size_t)m_StatStg.cbSize.QuadPart );
      
      // now free the old buffer
      delete [] m_pBuffer;

      // assign the new one
      m_pBuffer  = pNewBuffer;
      m_pCurrPos = m_pBuffer + (sint32)m_StatStg.cbSize.QuadPart;

      m_StatStg.cbSize.QuadPart += cb;

      // and we should be ok to continue writing
   }

   memcpy( m_pCurrPos, pv, cb );
   m_pCurrPos += cb;

   if ( pcbWritten )
   {
      *pcbWritten = cb;
   }

   return Ok;
}
