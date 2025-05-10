#include "ZorgeThread.h"
#include <errno.h>
#include <stdio.h>

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CZorgeThread::CZorgeThread(size_t nStackSize)
{
	m_hThread = 0;
	m_nStackSizeBytes = nStackSize;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CZorgeThread::~CZorgeThread()
{
}

//--------------------------------------------------------------------
// return false if failed.
//--------------------------------------------------------------------
bool CZorgeThread::SetStackSize()
{
	if(m_nStackSizeBytes < (size_t)PTHREAD_STACK_MIN)
	{
		printf("StackSize(). Forcing value of %ld to min stack size %ld bytes.", m_nStackSizeBytes, PTHREAD_STACK_MIN);
		m_nStackSizeBytes = PTHREAD_STACK_MIN;
	}

	int nRc = pthread_attr_setstacksize(&m_Attr, m_nStackSizeBytes);
	if(nRc != 0)
	{
		printf("pthread_attr_setstacksize failed with errno = %d, value = %lu\n", errno, (unsigned long)m_nStackSizeBytes);
		return false;
	}
	return true;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
bool CZorgeThread::Start(TEntryPoint EntryPoint, void* pParam)
{
    int nRc = pthread_attr_init(&m_Attr);
	if(nRc != 0)
	{
		printf("pthread_attr_init failed with errno = %d\n", errno);
		return false;
	}

	if(m_nStackSizeBytes != THREAD_STACK_DEFAULT)
		SetStackSize();

	nRc = pthread_attr_setdetachstate(&m_Attr, PTHREAD_CREATE_DETACHED);
	if(nRc != 0)
	{
		printf("pthread_attr_setdetachstate failed with errno = %d\n", errno);
		return false;
	}

	size_t nStackSizeBytes = 0;
	nRc = pthread_attr_getstacksize(&m_Attr, &nStackSizeBytes);
	//printf("nStackSize = %ld, min = %ld\n", nStackSizeBytes, PTHREAD_STACK_MIN);

	nRc = pthread_create(&m_hThread, &m_Attr, EntryPoint, pParam);
	if(nRc != 0)
	{
		printf("pthread_create failed with errno = %d\n", errno);
	}

    nRc = pthread_attr_destroy(&m_Attr);
	if(nRc != 0)
	{
		printf("pthread_attr_destroy failed with errno = %d\n", errno);
		return false;
	}

    if(m_hThread <= 0)
    {
		puts("Failed to start thread");
        return false; // failed to create thread !
    }

    return true;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
unsigned long CZorgeThread::GetId() const
{
	return (unsigned long)m_hThread;
	//return (unsigned long) pthread_self();
}
