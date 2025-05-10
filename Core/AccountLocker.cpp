#include "AccountLocker.h"
#include "SysUtils.h"

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
CAccountLocker::CAccountLocker()
{
	m_bShutDown = false;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
CAccountLocker::~CAccountLocker()
{
	m_bShutDown = true;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void CAccountLocker::ShutDown()
{
	m_bShutDown = true;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
int CAccountLocker::Lock(const CZorgeString& strNum)
{
	while(true)
	{
		if(m_bShutDown)
			return LOCK_SHUTDOWN;

		m_Mutex.Lock();
		TMapLocker::iterator it = m_Map.find(strNum);
		if(it == m_Map.end())
		{
			m_Map.insert(make_pair(strNum, 0));
			m_Mutex.Unlock();
			return LOCK_OK;
		}
		else // strNum still locked.
		{
			m_Mutex.Unlock();
			ZorgeSleep(10);
		}
	}

	printf("Assert true at %s %d\n", __FILE__, __LINE__);
	return 2024; // never gets here
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
int CAccountLocker::Unlock(const CZorgeString& strNum)
{
	if(m_bShutDown)
		return LOCK_SHUTDOWN;

	CMutexGuard G(m_Mutex);
	TMapLocker::iterator it = m_Map.find(strNum);
	if(it != m_Map.end())
		m_Map.erase(it);
	return LOCK_OK;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void CAccountLocker::Dump()
{
	if(m_bShutDown)
		return;

	CMutexGuard G(m_Mutex);
	printf("CAccountLocker size() = %ld\n", m_Map.size());
	TMapLocker::iterator it = m_Map.begin();
	for(; it != m_Map.end(); ++it)
		printf("[%s]\n", (const char*)(*it).first);
}

/*
struct C1
{
	C1()
	{
		CAccountLocker L;
		CZorgeString s("12");
		L.Lock(s);
		//L.Dump();
		//L.Unlock(s);
		//L.Dump();
	}
} g11;
*/
