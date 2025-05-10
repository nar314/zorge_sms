// ZorgeMutex.h

#ifndef CORE_ZORGE_MUTEX_H_
#define CORE_ZORGE_MUTEX_H_

#include <pthread.h>

#define MUTEX_RC_OK	0

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
class CZorgeMutex
{
    pthread_mutex_t		m_Mutex;
    pthread_mutexattr_t m_MutexAttr;

public:
	CZorgeMutex(const CZorgeMutex&) = delete;
	CZorgeMutex& operator=(const CZorgeMutex&) = delete;

	CZorgeMutex();
	~CZorgeMutex();

    int    Lock();
    int    Unlock();
    pthread_mutex_t& GetNativeMutex();
};

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
class CMutexGuard
{
	CZorgeMutex* m_pMutex;
public:
	CMutexGuard(const CMutexGuard&) = delete;
	CMutexGuard operator=(const CMutexGuard&) = delete;

	CMutexGuard(CZorgeMutex&);
	~CMutexGuard();
};

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
class CScopeGuard
{
	bool& m_bValue;
public:
	CScopeGuard(const CScopeGuard&) = delete;
	CScopeGuard operator=(const CScopeGuard&) = delete;

	CScopeGuard(bool& b) : m_bValue(b)
	{
		m_bValue = true;
	}
	~CScopeGuard()
	{
		m_bValue = false;
	}
};
#endif
