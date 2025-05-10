#include "OpenConsCache.h"
#include "SysUtils.h"
#include <atomic>

atomic<unsigned int> g_COpenConsItem = 0;
atomic<unsigned int> g_COpenConsItem_max = 0;

struct CCount3
{
	~CCount3()
	{
		printf("g_COpenConsItem = %u (%u)\n", (unsigned int)g_COpenConsItem, (unsigned int)g_COpenConsItem_max);
	}
} g_Count3;

COpenConsItem::COpenConsItem()
{
	m_Status = NotFound;
	m_nLastAccess = 0;
	m_nSeqNum = 0;
	m_bError = false;
	g_COpenConsItem++;
	g_COpenConsItem_max++;
}

COpenConsItem::~COpenConsItem()
{
	g_COpenConsItem--;
	//printf("~COpenConsItem, %s, status=%d, time=%ld\n", m_szClientId, m_Status, m_nLastAccess);
}

//-------------------------------------------------------------
//
//-------------------------------------------------------------
COpenConsCache::COpenConsCache()
{
	m_nMaxLiveSec = 60;
	m_pOldestItem = 0;
	m_nExpiredCalled = 0;
	m_bTrace = false;
}

//-------------------------------------------------------------
//
//-------------------------------------------------------------
COpenConsCache::~COpenConsCache()
{
	if(m_Map.size() == 0)
		return;

	TOpenItems::iterator it = m_Map.begin();
	for(; it != m_Map.end(); ++it)
		delete (*it).second;
	m_Map.clear();
}

//-------------------------------------------------------------
//
//-------------------------------------------------------------
void COpenConsCache::Add(const char* szClientId, unsigned int nSeqNum)
{
	if(szClientId == 0 || *szClientId == 0 || nSeqNum == 0)
		return; // error

	CMutexGuard Lock(m_Mutex);
	TOpenItems::iterator it = m_Map.find(szClientId);
	time_t nTimeCur = time(0);
	if(it == m_Map.end())
	{
		COpenConsItem* p = new COpenConsItem();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
		strncpy(p->m_szClientId, szClientId, sizeof(p->m_szClientId));
#pragma GCC diagnostic pop
		p->m_nSeqNum = nSeqNum;
		p->m_nLastAccess = nTimeCur;
		p->m_strReply.Empty();
		p->m_Status = InProcessing;

		if(m_pOldestItem == 0)
			m_pOldestItem = p;
		m_Map.insert(make_pair(p->m_szClientId, p));
	}
	else
	{

		// Update existing item
		COpenConsItem* pExisting = (*it).second;
		pExisting->m_Status = InProcessing;
		pExisting->m_nSeqNum = nSeqNum;
		time_t nOrigTime = pExisting->m_nLastAccess;
		pExisting->m_nLastAccess = nTimeCur;
		pExisting->m_strReply.Empty();
		pExisting->m_bError = false;

		if(m_pOldestItem == pExisting && (nOrigTime - nTimeCur) > m_nMaxLiveSec / 2)
			FindOldestItem();
	}

	if(nTimeCur - m_nExpiredCalled >= m_nMaxLiveSec)
		RemoveExpired("Add");
}

//-------------------------------------------------------------
//
//-------------------------------------------------------------
void COpenConsCache::Update(const char* szClientId, unsigned int nSeqNum, bool bError, const CZorgeString& strReply)
{
	if(szClientId == 0 || *szClientId == 0 || nSeqNum == 0)
		return; // error

	CMutexGuard Lock(m_Mutex);
	TOpenItems::iterator it = m_Map.find(szClientId);
	time_t nTimeCur = time(0);
	if(it == m_Map.end())
	{
		// Why it is missing ?
		COpenConsItem* p = new COpenConsItem();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
		strncpy(p->m_szClientId, szClientId, sizeof(p->m_szClientId));
#pragma GCC diagnostic pop
		p->m_Status = Done;
		p->m_nSeqNum = nSeqNum;
		p->m_nLastAccess = nTimeCur;
		p->m_strReply = strReply;
		p->m_bError = bError;

		if(m_pOldestItem == 0)
			m_pOldestItem = p;
		m_Map.insert(make_pair(p->m_szClientId, p));
	}
	else
	{
		// Update existing item. No need to check if expired.
		COpenConsItem* pExisting = (*it).second;
		pExisting->m_Status = Done;
		pExisting->m_nSeqNum = nSeqNum; // it is faster to copy then to compare
		time_t nOrigTime = pExisting->m_nLastAccess;
		pExisting->m_nLastAccess = nTimeCur;
		pExisting->m_strReply = strReply;
		pExisting->m_bError = bError;

		// Updating oldest item.
		if(m_pOldestItem == pExisting && (nOrigTime - nTimeCur) > m_nMaxLiveSec / 2)
			FindOldestItem();
	}

	if(nTimeCur - m_nExpiredCalled >= m_nMaxLiveSec)
		RemoveExpired("Update");
}

//-------------------------------------------------------------
//
//-------------------------------------------------------------
CacheItemStatus	COpenConsCache::Find(const char* szClientId, unsigned int nSeqNum, bool& bError, CZorgeString& strReply)
{
	CMutexGuard Lock(m_Mutex);
	TOpenItems::iterator it = m_Map.find(szClientId);
	if(it == m_Map.end())
		return NotFound; // Client id not found.

	COpenConsItem* p = (*it).second;
	if(p->m_nSeqNum != nSeqNum)
		return NotFound; // Client id found, but wrong sequence number

	CacheItemStatus RetStat = p->m_Status;
	time_t nTimeCur = time(0);
	if(nTimeCur - p->m_nLastAccess >= m_nMaxLiveSec)
	{
		RetStat = NotFound; // Expired
		RemoveExpired("Find");
	}
	else
	{
		p->m_nLastAccess = nTimeCur;
		strReply = p->m_strReply;
		bError = p->m_bError;
	}

	return RetStat;
}

//-------------------------------------------------------------
//
//-------------------------------------------------------------
void COpenConsCache::Print()
{
	CMutexGuard Lock(m_Mutex);
	CZorgeString strTmp;
	time_t nOldestTime = m_pOldestItem ? m_pOldestItem->m_nLastAccess : 0;
	printf("--> COpenConsCache::Print() size=%ld, TTL=%ld sec, oldestTime=%s\n", m_Map.size(), m_nMaxLiveSec, TimeToString(nOldestTime, strTmp));
	TOpenItems::const_iterator it = m_Map.begin();
	for(; it != m_Map.end(); ++it)
	{
		//printf("key=%s, ", (*it).first);
		(*it).second->Print();
	}
	puts("<--");
}

//-------------------------------------------------------------
//
//-------------------------------------------------------------
void COpenConsCache::RemoveExpired(const char* szCaller)
{
	CMutexGuard Lock(m_Mutex);
	time_t nTimeCur = time(0);
	if(nTimeCur - m_nExpiredCalled < m_nMaxLiveSec)
	{
		if(m_bTrace)
			puts("RemoveExpired(). Calling too soon.");
		return; // Calling is too soon.
	}

	m_nExpiredCalled = nTimeCur;
	if(m_Map.size() == 0)
	{
		if(m_bTrace)
			puts("RemoveExpired(). size=0");
		return; // Nothing to work with
	}

	unsigned int nCount = 0;
	time_t t = 0;
	TOpenItems::iterator it = m_Map.begin();
	while(it != m_Map.end())
	{
		COpenConsItem* p = (*it).second;
		t = nTimeCur - p->m_nLastAccess;
		if(t >= m_nMaxLiveSec)
		{
			++nCount;
			delete p;
			it = m_Map.erase(it);
		}
		else
			++it;
	}

	FindOldestItem();
	if(m_bTrace)
	{
		CZorgeString strTmp;
		time_t nOldestTime = m_pOldestItem ? m_pOldestItem->m_nLastAccess : 0;
		printf("RemoveExpired(%s), expired=%d, size=%ld, TTL=%ld sec, %s\n", szCaller, nCount, m_Map.size(), m_nMaxLiveSec, TimeToString(nOldestTime, strTmp));
	}
}

// private
void COpenConsCache::FindOldestItem()
{
	m_pOldestItem = 0;
	time_t nOldestTime = m_pOldestItem ? m_pOldestItem->m_nLastAccess : 0;
	TOpenItems::iterator it = m_Map.begin();
	for(; it != m_Map.end(); ++it)
	{
		COpenConsItem* p = (*it).second;
		if(p->m_nLastAccess > nOldestTime)
			m_pOldestItem = p;
	}
}

//-------------------------------------------------------------
//
//-------------------------------------------------------------
void COpenConsCache::SetTrace(bool b)
{
	m_bTrace = b;
	printf("RemoveExpired() Trace set to %d\n", b);
}

//-------------------------------------------------------------
//
//-------------------------------------------------------------
void COpenConsCache::SetTTL(unsigned int nSec)
{
	m_nMaxLiveSec = nSec;
	if(m_bTrace)
		printf("RemoveExpired() TTL set to %d sec.\n", nSec);
}

//-------------------------------------------------------------
//
//-------------------------------------------------------------
void COpenConsCache::Diagnostics(CZorgeString& strOut)
{
	CMutexGuard Lock(m_Mutex);

	CZorgeString strTmp;
	time_t nOldestTime = m_pOldestItem ? m_pOldestItem->m_nLastAccess : 0;
	strTmp.Format("OpenConCache. size=%ld, Cur=%u, Max=%u, TTL=%ld sec, oldestTime=%s\n",
			m_Map.size(), (unsigned int)g_COpenConsItem, (unsigned int)g_COpenConsItem_max, m_nMaxLiveSec, TimeToString(nOldestTime, strTmp));
	strOut += strTmp;
}
