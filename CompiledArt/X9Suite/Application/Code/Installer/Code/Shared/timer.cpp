
#include "timer.h"

__int64        Timer::m_pause64;
__int64			Timer::m_total64;
__int64			Timer::m_second64;
__int64			Timer::m_zero64;
__int64			Timer::m_delta64;
__int64			Timer::m_last64;

float			Timer::m_total;
float			Timer::m_rate;
float			Timer::m_delta;

HRESULT Timer::Create ( void )
{
	BOOL success;
	
	//How many cpu ticks in a second?
	success = QueryPerformanceFrequency( (LARGE_INTEGER *) &m_second64 );

	if ( !success ) return E_FAIL;
	

	//How many ticks have gone by within this second?
	QueryPerformanceCounter( (LARGE_INTEGER *) &m_zero64 );
	
	return Update ( );
}

HRESULT Timer::Destroy ( void )
{

	return S_OK;
}

HRESULT Timer::Pause( )
{
   Update( );

   // mark where we pause
   m_pause64 = m_total64;

   return S_OK;
}

HRESULT Timer::UnPause( )
{
   Update( );

   // add the amount of time we were paused so that it's as if the time never happened.
   m_zero64 += m_total64 - m_pause64;

   Update( );

   return S_OK;
}

HRESULT Timer::Update ( void )
{
	__int64 current64;
	
	//Where is the cpu in the second?
	QueryPerformanceCounter( (LARGE_INTEGER *) &current64 );
		

	//_control87( _PC_53, _MCW_PC );

		m_total64 = current64 - m_zero64;

		m_total = static_cast <float> ( m_total64 ) / static_cast <float> ( m_second64 );

		m_delta64 = m_total64 - m_last64; 

		m_delta = static_cast <float> ( m_delta64 );

		m_rate = (float) m_second64 / m_delta64;

		m_last64 = m_total64;

	//_control87( _PC_24, _MCW_PC );


	return S_OK;
}
