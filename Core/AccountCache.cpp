#include <algorithm>    // std::sort

#include "AccountCache.h"
#include "ZorgeError.h"
#include "ZorgeTimer.h"
#include "SysUtils.h"

std::atomic<unsigned int> g_CAccountCacheItem = 0;
std::atomic<unsigned int> g_CAccountCacheItem_max = 0;

#define DEFAULT_TIMEOUT_SEC 5

void DiagCAccountCacheItem(CZorgeString& strOut)
{
	CZorgeString str;
	str.Format("CAccountCacheItem. Cur=%d, Max=%d\n", (unsigned int)g_CAccountCacheItem, (unsigned int)g_CAccountCacheItem_max);
	strOut += str;
}

struct CAccountCacheItemCounter
{
	~CAccountCacheItemCounter()
	{
		printf("g_CAccountCacheItem = %d (%d)\n", (unsigned int)g_CAccountCacheItem, (unsigned int)g_CAccountCacheItem_max);
	}
	static void Add()
	{
		g_CAccountCacheItem++;
		g_CAccountCacheItem_max++;
	}
} gCAccountCacheItemCounter;

CAccountCacheItem::CAccountCacheItem(CAccount* pAccount)
{
	CAccountCacheItemCounter::Add();

	m_LockedBy = 0;
	m_LastAccess = 0;
	m_bLocked = false;
	m_pAccount = pAccount;

	//puts("CAccountCacheItem");
	//Print();
}

CAccountCacheItem::~CAccountCacheItem()
{
	//puts("~CAccountCacheItem");
	//Print();

	g_CAccountCacheItem--;
	if(m_pAccount)
		delete m_pAccount;
}

CAccount* CAccountCacheItem::GetAccount()
{
	m_LastAccess = time(0);
	//puts("GetAccount");
	//Print();
	return m_pAccount;
}

void CAccountCacheItem::Lock()
{
	m_LockedBy = pthread_self();
	m_LastAccess = time(0);
	if(m_bLocked)
		puts("Assert(true) Double locking account cache item.");
	m_bLocked = true;

	//puts("Lock");
	//Print();
}

void CAccountCacheItem::Release()
{
	m_LockedBy = 0;
	m_bLocked = false;
}

void CAccountCacheItem::Print()
{
	printf("[CAccountCacheItem] lock=%d, time=%ld thread=%lX, %s\n", m_bLocked, m_LastAccess, m_LockedBy, m_pAccount ? (const char*)m_pAccount->GetNum() : "NULL");
}

#define DEFAULT_MAX_MEM_KB 		(1 * 1024) // 1 MB
#define DEFAULT_MIN_MEM_KB 		256 // 256 KB
#define DEFAULT_MIN_ITEMS_ACCOUNT_CACHE 100
//-------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------
CAccountCache::CAccountCache()
{
	m_nMaxMemory = m_nMaxItems = m_nDeleted = 0;
	m_nTimeOutSec = 0;

	SetLimits(DEFAULT_MAX_MEM_KB, DEFAULT_TIMEOUT_SEC);
}

//-------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------
CAccountCache::~CAccountCache()
{
	Clean();
}

//-------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------
void CAccountCache::SetLimits(unsigned long nMaxMemKB, unsigned int nTimeOutSec)
{
	if(nTimeOutSec < 1)
	{
		printf("CBlockCache. Timeout has invalid value '%d'. Forcing default value %d KB\n", nTimeOutSec, DEFAULT_TIMEOUT_SEC);
		nTimeOutSec = DEFAULT_TIMEOUT_SEC;
	}

	m_nTimeOutSec = nTimeOutSec;

	if(nMaxMemKB < DEFAULT_MIN_MEM_KB)
	{
		printf("CAccountCache. Max memory reaches the max. Forcing default value %d KB\n", DEFAULT_MAX_MEM_KB);
		nMaxMemKB = DEFAULT_MAX_MEM_KB;
	}

	m_nMaxMemory = nMaxMemKB * 1024;
	m_nMaxItems = m_nMaxMemory / sizeof(CAccount);
	if(m_nMaxItems < DEFAULT_MIN_ITEMS_ACCOUNT_CACHE)
	{
		printf("CAccountCache. Warning. m_nMaxItems less than default value. Forcing default value %d.\n", DEFAULT_MIN_ITEMS_ACCOUNT_CACHE);
		m_nMaxItems = DEFAULT_MIN_ITEMS_ACCOUNT_CACHE;
	}
	printf("CAccountCache. m_nMaxMemory=%ld KB, m_nMaxItems=%ld, timeout=%d sec\n", m_nMaxMemory / 1024, m_nMaxItems, m_nTimeOutSec);
}

//-------------------------------------------------------------------------------------
// Storage calling it during login.
//-------------------------------------------------------------------------------------
void CAccountCache::Clean()
{
	CMutexGuard G(m_Lock);
	if(!m_mAccounts.size())
		return;

	TAccounts::iterator it = m_mAccounts.begin();
	for(; it != m_mAccounts.end(); ++it)
	{
		delete (*it).first;
		delete (*it).second;
	}
	m_mAccounts.clear();
}

//-------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------
CAccountCacheItem* CAccountCache::AddAndLock(CAccount* pAccount)
{
	if(!pAccount)
		throw CZorgeAssert("Assert(pAccount) at CAccountCache::Add");

	CMutexGuard G(m_Lock);
	const CZorgeString* pNum = new CZorgeString(pAccount->GetNum());
	CAccountCacheItem* pItem = new CAccountCacheItem(pAccount);
	pItem->Lock();
	m_mAccounts.insert(make_pair(pNum, pItem));
	return pItem;
}

//-------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------
void CAccountCache::Delete(const CZorgeString* pNum)
{
	if(!pNum)
		throw CZorgeAssert("Assert(pAccount) at CAccountCache::Delete");

	time_t nStart = time(0);
	pthread_t nThread = pthread_self();
	bool bToBeDeleted = false;

	while(1)
	{
		// Scope for guard
		{
			CMutexGuard G(m_Lock);
			TAccounts::iterator it = m_mAccounts.find(pNum);
			if(it == m_mAccounts.end())
				return; // Account not found

			CAccountCacheItem* pItem = (*it).second;
			if(!pItem->m_bLocked)
			{
				bToBeDeleted = true; // Account found and not locked.
			}
			else if(pItem->m_LockedBy == nThread)
			{
				bToBeDeleted = true; // Account found and locked by me
			}

			if(bToBeDeleted)
			{
				delete (*it).first;
				delete (*it).second;
				m_mAccounts.erase(it);
				++m_nDeleted;

				if(m_nMaxItems > 0 && m_nDeleted >= (m_nMaxItems / 2))
				{
					m_nDeleted = 0;
					Release();
				}
				return;
			}
			// Account locked, wait and try again or timeout
		}
		puts("CAccountCache::Delete waiting");
		ZorgeSleep(100);
		time_t nTotal = time(0) - nStart;
		if(nTotal >= m_nTimeOutSec)
			throw CZorgeError("Account cache. core is busy.");
	}
}

//-------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------
void CAccountCache::Print()
{
	CMutexGuard G(m_Lock);
	printf("CAccountCache. m_nMaxMemory = %ld KB, m_nMaxItems = %ld, size() = %ld\n", m_nMaxMemory / 1024, m_nMaxItems, m_mAccounts.size());
/*
	TAccounts::const_iterator it = m_mAccounts.begin();
	for(; it != m_mAccounts.end(); ++it)
	{
		printf("%s\n", (const char*)*(*it).first);
	}
*/
}

typedef pair<CAccountCacheItem*, const CZorgeString*> TA1;
static bool CompareItems(TA1& p1, TA1& p2)
{
	// ASC by last time access.
	return p1.first->m_LastAccess < p2.first->m_LastAccess;
}

// private
void CAccountCache::Release()
{
	//puts("-->CAccountCache::Release()");
	size_t nSizeBefore = m_mAccounts.size();
	if(nSizeBefore < m_nMaxItems)
		return;

	typedef pair<CAccountCacheItem*, const CZorgeString*> T1;
	typedef vector<T1> TSort;
	TSort vSort;

	CAccountCacheItem* pItem = 0;
	const CZorgeString* strKey = 0;
	TAccounts::iterator it = m_mAccounts.begin();
	for(; it != m_mAccounts.end(); ++it)
	{
		pItem = (*it).second;
		strKey = (*it).first;
		if(!pItem->m_bLocked)
			vSort.push_back(make_pair(pItem, strKey));
	}

	// I got only not locked accounts
	TSort::iterator itV;
	sort(vSort.begin(), vSort.end(), CompareItems);

	unsigned int nLeft = nSizeBefore / 3;
	for(itV = vSort.begin(); itV != vSort.end(); ++itV, --nLeft)
	{
		if(nLeft == 0)
			break;

		strKey = (*itV).second;

		TAccounts::iterator itM = m_mAccounts.find(strKey);
		if(itM != m_mAccounts.end())
		{
			delete (*itM).first;
			delete (*itM).second;
			m_mAccounts.erase(itM);
		}
		else
			printf("Not found in cache for release handles %s\n", (const char*)(*strKey));
	}
}

//-------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------
CAccountCacheItem* CAccountCache::Find(const CZorgeString* pNum)
{
	if(!pNum)
		throw CZorgeAssert("Assert(pAccount) at CAccountCache::Find");

	time_t nStart = time(0);
	pthread_t nThread = pthread_self();

	while(1)
	{
		// Scope for guard
		{
			CMutexGuard G(m_Lock);
			TAccounts::iterator it = m_mAccounts.find(pNum);
			if(it == m_mAccounts.end())
				return 0;

			CAccountCacheItem* pItem = (*it).second;
			if(!pItem->m_bLocked)
			{
				pItem->Lock();
				return pItem;
			}
			if(pItem->m_LockedBy == nThread)
				return pItem;

			// Account locked, wait and try again or timeout
		}
		//printf("CAccountCache::Find %lx waiting for %s\n", pthread_self(), (const char*)*pNum);
		ZorgeSleep(100);
		time_t nTotal = time(0) - nStart;
		if(nTotal >= m_nTimeOutSec)
			throw CZorgeError("Account cache. Core is busy.");
	}
	return 0;
}

//-------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------
void CAccountCache::Diagnostics(CZorgeString& strOut)
{
	CMutexGuard G(m_Lock);
	CZorgeString strTmp;
	strTmp.Format("AccountCache. size=%ld, Cur=%d, Max=%d, MaxMem=%ld KB, MaxItems=%ld\n",
			m_mAccounts.size(), (unsigned int)g_CAccountCacheItem, (unsigned int)g_CAccountCacheItem_max, m_nMaxMemory / 1024, m_nMaxItems);
	strOut += strTmp;
}
