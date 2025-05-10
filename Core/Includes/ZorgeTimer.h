// ZorgeTimer.h

#ifndef CORE_ZORGETIMER_H_
#define CORE_ZORGETIMER_H_

#include <sys/time.h>

class CZorgeTimer
{
	struct timeval m_Start, m_End;
	bool m_bStop;
public:
	CZorgeTimer(const CZorgeTimer&) = delete;
	CZorgeTimer& operator=(const CZorgeTimer&) = delete;

	CZorgeTimer();

	void	Start();
	void 	Stop();
	long 	GetMilliSecs(); // 1000 ms = 1 sec
	long 	GetSecs();
	const char* ToString(char*, int);
};

#endif
