#include "precompiled.h"
#include "utilityClock.h"

float Clock::MarkSample( void )
{
   float time = TestSample( );

   Reset( );

   return time;
}

#ifdef WIN32
void  Clock::Start( void )
{
   QueryPerformanceFrequency( (LARGE_INTEGER *) &m_Frequency );

   Reset( );
}

float Clock::TestSample( void )
{
   LARGE_INTEGER counter;

   QueryPerformanceCounter( &counter );
   
   counter.QuadPart = counter.QuadPart - m_Zero;

   return (float) (counter.QuadPart / (double) m_Frequency);
}

void Clock::Reset( void )
{
   QueryPerformanceCounter( (LARGE_INTEGER *) &m_Zero );
}

#elif defined IPHONE
void Clock::Start( void )
{
   m_Frequency = 1000000;

   Reset( );
}

float Clock::TestSample( void )
{
   timeval t;
   struct timezone tz;

   gettimeofday( &t, &tz );

   t.tv_sec  = t.tv_sec - m_Zero.tv_sec;

   //place in a signed int so it can be negative (e.g. now = 1.0, base = 0.9)
   sint64 usec = t.tv_usec - m_Zero.tv_usec;

   uint64 nowTime = t.tv_sec * m_Frequency + usec;

   return (float) (nowTime / (double) m_Frequency);
}

void Clock::Reset( void )
{
   struct timezone tz;

   gettimeofday( &m_Zero, &tz );
}

#elif defined LINUX
void Clock::Start( void )
{
   m_Frequency = 1000000000;

   Reset( );
}

float Clock::TestSample( void )
{
   timespec t;
   clock_gettime( CLOCK_REALTIME, &t );

   t.tv_sec  = t.tv_sec - m_Zero.tv_sec;

   //place in a signed int so it can be negative (e.g. now = 1.0, base = 0.9)
   sint64 nsec = t.tv_nsec - m_Zero.tv_nsec;

   uint64 nowTime = t.tv_sec * m_Frequency + nsec;

   return (float) (nowTime / (double) m_Frequency);
}

void Clock::Reset( void )
{
   clock_gettime( CLOCK_REALTIME, &m_Zero );
}
#else
   #error "Platform not defined"
#endif
