
#ifndef IMAGE_STREAM_H_
#define IMAGE_STREAM_H_

class ImageStream : public IStream
{
   public:
                                     ImageStream   ( void );
                                     ~ImageStream  ( );

   virtual HRESULT STDMETHODCALLTYPE Read          ( void *pv, ULONG cb, ULONG *pcbRead );
   virtual HRESULT STDMETHODCALLTYPE Write         ( const void *pv, ULONG cb, ULONG *pcbWritten );
   virtual HRESULT STDMETHODCALLTYPE Seek          ( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition );
   virtual HRESULT STDMETHODCALLTYPE Stat          ( STATSTG *pstatstg, DWORD grfStatFlag );
   virtual HRESULT STDMETHODCALLTYPE SetSize       ( ULARGE_INTEGER libNewSize );
   virtual ULONG   STDMETHODCALLTYPE AddRef        ( void );
   virtual ULONG   STDMETHODCALLTYPE Release       ( void );
   virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject );
        
   //Not implemented because we don't need to support them. If they are ever called, we'll throw an assert so we can implement.
   virtual HRESULT STDMETHODCALLTYPE CopyTo        ( IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten ) { _ASSERTE( 0 ); return S_OK; }
   virtual HRESULT STDMETHODCALLTYPE Commit        ( DWORD grfCommitFlags )                                                                  { _ASSERTE( 0 ); return S_OK; }
   virtual HRESULT STDMETHODCALLTYPE Revert        ( void )                                                                                  { _ASSERTE( 0 ); return S_OK; }
   virtual HRESULT STDMETHODCALLTYPE LockRegion    ( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType )                         { _ASSERTE( 0 ); return S_OK; }
   virtual HRESULT STDMETHODCALLTYPE UnlockRegion  ( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType )                         { _ASSERTE( 0 ); return S_OK; }
   virtual HRESULT STDMETHODCALLTYPE Clone         ( IStream **ppstm )                                                                       { _ASSERTE( 0 ); return S_OK; }

   protected:
           sint32   m_RefCount;
           STATSTG  m_StatStg;
           BYTE    *m_pBuffer;
           BYTE    *m_pCurrPos;
        
};

#endif
