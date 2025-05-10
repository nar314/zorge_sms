// ZorgeThread.h

#ifndef CORE_ZORGE_THREAD_H_
#define CORE_ZORGE_THREAD_H_

#include <pthread.h>

struct CRunningGuard
{
	CRunningGuard(const CRunningGuard&) = delete;
	CRunningGuard operator=(const CRunningGuard&) = delete;

	bool& m_bRunning;
	CRunningGuard(bool& b) : m_bRunning(b)
	{
		m_bRunning = true;
	}
	~CRunningGuard()
	{
		m_bRunning = false;
	}
};

typedef void*(*TEntryPoint)(void*);
#define THREAD_STACK_DEFAULT 0

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
class CZorgeThread
{
    pthread_t		m_hThread;
    pthread_attr_t	m_Attr;
    size_t			m_nStackSizeBytes;

    bool			SetStackSize();

public:
	CZorgeThread(const CZorgeThread&) = delete;
	CZorgeThread& operator=(const CZorgeThread&) = delete;

	CZorgeThread(size_t = THREAD_STACK_DEFAULT);
	~CZorgeThread();

    bool			Start(TEntryPoint, void* = 0);
    unsigned long 	GetId() const;
    //bool			IsRunning() const;
};

#endif
