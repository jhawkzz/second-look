#ifndef _TIMER_
#define _TIMER_

#include <windows.h>

class Timer
{
public:
	
	static	HRESULT			Create	( void );
	static	HRESULT			Destroy	( void );
	static	HRESULT			Update	( void );
   static   HRESULT        Pause    ( void );
   static   HRESULT        UnPause  ( void );

public:
	
	static	float			m_total;
	static	float			m_rate;
	static	float			m_delta;

private:

   static   __int64        m_pause64;
	static	__int64			m_total64;
	static	__int64			m_second64;
	static	__int64			m_zero64;
	static	__int64			m_delta64;
	static	__int64			m_last64;

};


#endif
