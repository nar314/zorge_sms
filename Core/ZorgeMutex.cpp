#include "ZorgeMutex.h"
#include <stdio.h>
#include <stdlib.h>

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CZorgeMutex::CZorgeMutex()
{
	pthread_mutexattr_init(&m_MutexAttr);
	pthread_mutexattr_settype(&m_MutexAttr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&m_Mutex, &m_MutexAttr);
}

CZorgeMutex::~CZorgeMutex()
{
	pthread_mutexattr_destroy(&m_MutexAttr);
	pthread_mutex_destroy(&m_Mutex);
}

int CZorgeMutex::Lock()
{
    int n = pthread_mutex_lock(&m_Mutex);
    //printf("CZorgeMutex::Lock n=%d [this = %X] {thread %X} =======> Locked, native = %X\n", n, this, pthread_self(), m_Mutex);
    return n;
}

int CZorgeMutex::Unlock()
{
    int n = pthread_mutex_unlock(&m_Mutex);
    //printf("CZorgeMutex::Unlock n=%d [this = %X] {thread %X} =======> Locked, native = %X\n", n, this, pthread_self(), m_Mutex);
    return n;
}

pthread_mutex_t& CZorgeMutex::GetNativeMutex()
{
	return m_Mutex;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CMutexGuard::CMutexGuard(CZorgeMutex& mMutex)
{
	m_pMutex = &mMutex;
	int n = m_pMutex->Lock();
	//printf("[mutex = %X] {thread %ld} =======> Locked, native = %X\n", m_pMutex, pthread_self(), m_pMutex->GetNativeMutex());
	if(n != 0)
	{
		printf("Mutex failed to lock. n = %d\n", n);
		//abort();
	}
}

CMutexGuard::~CMutexGuard()
{
	int n = m_pMutex->Unlock();
	//printf("[mutex = %X] {thread %ld} =======> Unlocked, native = %X\n", m_pMutex, pthread_self(), m_pMutex->GetNativeMutex());
	if(n != 0)
	{
		printf("Mutex failed to unlock. n = %d\n", n);
		//abort();
	}
}

