// AccountLocker.h

#ifndef CORE_ACCOUNTLOCKER_H_
#define CORE_ACCOUNTLOCKER_H_

#include <map>
#include "ZorgeString.h"
#include "ZorgeMutex.h"

using namespace std;

typedef map<CZorgeString, char> TMapLocker;

#define LOCK_OK 		0
#define LOCK_SHUTDOWN	1
//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
class CAccountLocker
{
	TMapLocker	m_Map;
	bool		m_bShutDown;
	CZorgeMutex	m_Mutex;
public:
	CAccountLocker(const CAccountLocker&) = delete;
	CAccountLocker operator=(const CAccountLocker&) = delete;

	CAccountLocker();
	~CAccountLocker();

	void	ShutDown();
	int		Lock(const CZorgeString&);
	int		Unlock(const CZorgeString&);
	void	Dump();
};

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
struct CLockerGuard
{
	CAccountLocker& 	m_Locker;
	const CZorgeString&	m_strNum;

	CLockerGuard(CAccountLocker& Locker, const CZorgeString& strNum) :
		m_Locker(Locker), m_strNum(strNum)
	{
		int n = m_Locker.Lock(m_strNum);
		if(n != LOCK_OK)
			printf("CLockerGuard failed to lock '%s', %d\n", (const char*)m_strNum, n);
	}
	~CLockerGuard()
	{
		int n = m_Locker.Unlock(m_strNum);
		if(n != LOCK_OK)
			printf("CLockerGuard failed to unlock '%s', %d\n", (const char*)m_strNum, n);
	}
};

#endif
