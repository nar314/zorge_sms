#include <string.h>
#include <stdio.h>

#include "ZorgeTimer.h"

CZorgeTimer::CZorgeTimer()
{
	m_bStop = false;
	memset(&m_End, 0, sizeof(timeval));
	gettimeofday(&m_Start, 0);
}

void CZorgeTimer::Start()
{
	m_bStop = false;
	gettimeofday(&m_Start, 0);
}

void CZorgeTimer::Stop()
{
	m_bStop = true;
	gettimeofday(&m_End, 0);
}

long CZorgeTimer::GetMilliSecs() // 1000 ms = 1 sec
{
	if(!m_bStop)
		Stop();

	long nSecs  = m_End.tv_sec - m_Start.tv_sec;
	long nUsecs = m_End.tv_usec - m_Start.tv_usec;

	return ((nSecs) * 1000 + nUsecs/1000.0) + 0.5;
}

long CZorgeTimer::GetSecs()
{
	if(!m_bStop)
		Stop();
	return (long)(m_End.tv_sec - m_Start.tv_sec); // loosing msecs
}

const char* CZorgeTimer::ToString(char* szOut, int nOutSize)
{
	long nDif = GetMilliSecs();
	unsigned int nSec = nDif / 1000;
	unsigned int nMin = nSec / 60;
	nSec -= nMin * 60;
	unsigned int nMs = nDif - (nSec * 1000 + nMin * 60 * 1000);

	snprintf(szOut, nOutSize, "%02d:%02d.%03d", nMin, nSec, nMs);
	return szOut;
}
