
#ifndef THREADLOCK_H_
#define THREADLOCK_H_

class Lock
{
   friend class ThreadLock;

private:
   HANDLE  m_hMutex;

public:
   Lock( void )
   {
      m_hMutex = CreateMutex( NULL, FALSE, NULL );
   }

   ~Lock( void )
   {
      CloseHandle( m_hMutex ); 
   }
};

class ThreadLock
{
protected:
   Lock *m_pLock;

public:
   ThreadLock( Lock &lock )
   {
      m_pLock = &lock;

      Acquire( *m_pLock, TRUE );
   }

   ~ThreadLock( void )
   {
      Release( *m_pLock );
   }

public:
   static BOOL Acquire( 
      Lock &lock, 
      BOOL block = TRUE 
   )
   {
      DWORD result = WaitForSingleObject( lock.m_hMutex, block ? INFINITE : 0 );      
      
      //returns true if we acquired the mutex
      return ( WAIT_OBJECT_0 == result );
   }

   static void Release(
      Lock &lock
   )
   {
      ReleaseMutex( lock.m_hMutex );
   }
};

#endif //THREADLOCK_H_
