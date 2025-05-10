#include <algorithm>    // std::sort
#include "Includes/BlocksCache.h"
#include "ZorgeError.h"
#include "SysUtils.h"
#include "ZorgeTimer.h"

#define MIN_HANDLES_OPEN 1000

#define DEFAULT_TIMEOUT_SEC 5
#define DEFAULT_MAX_MEM_KB 	(1 * 1024) // 1 MB
#define MIN_MEM_KB 			256 // 256 KB
#define DEFAULT_PERCENT 	10

std::atomic<unsigned int> g_CBlocksCacheItem = 0;
std::atomic<unsigned int> g_CBlocksCacheItem_max = 0;

static unsigned long g_nMaxAddedToCheckHandles = 512;

void DiagCBlocksCacheItem(CZorgeString& strOut)
{
	CZorgeString str;
	str.Format("CBlocksCacheItem. Cur=%d, Max=%d\n", (unsigned int)g_CBlocksCacheItem, (unsigned int)g_CBlocksCacheItem_max);
	strOut += str;
}

struct CBlocksCacheItemCounter
{
	~CBlocksCacheItemCounter()
	{
		printf("g_CBlocksCacheItem = %d (%d)\n", (unsigned int)g_CBlocksCacheItem, (unsigned int)g_CBlocksCacheItem_max);
	}
	static void Add()
	{
		g_CBlocksCacheItem++;
		g_CBlocksCacheItem_max++;
	}
} g_CBlocksCacheItemCounter;

CBlocksCacheItem::CBlocksCacheItem(CMsgBlock* pBlock)
{
	CBlocksCacheItemCounter::Add();

	m_LastAccess = 0;
	m_LockedBy = 0;
	m_bLocked = false;
	m_pBlock = pBlock;
}

CBlocksCacheItem::~CBlocksCacheItem()
{
	g_CBlocksCacheItem--;
	if(m_pBlock)
		delete m_pBlock;
}

CMsgBlock* CBlocksCacheItem::GetBlock()
{
	m_LastAccess = time(0);
	return m_pBlock;
}

void CBlocksCacheItem::Lock()
{
//	printf("CBlocksCacheItem::Lock() %X\n", m_pBlock);
	m_LastAccess = time(0);
	m_LockedBy = pthread_self();
	if(m_bLocked)
		puts("Assert(true) Double locking cache item.");
	m_bLocked = true;
}

void CBlocksCacheItem::Release()
{
	//printf("CBlocksCacheItem::Release() %X\n", m_pBlock);
	m_bLocked = false;
}

void CBlocksCacheItem::Print()
{
	printf("lock=%d, handle=%d, time=%ld, thread=%lX %s\n", m_bLocked, m_pBlock->IsHandleOpen(), m_LastAccess, m_LockedBy, (const char*)m_pBlock->m_strFullPath);
}

//------------------------------------------------------------------
//
//------------------------------------------------------------------
CBlocksCache::CBlocksCache()
{
	m_nMaxHandlesOpen = m_nTimeOutSec = 0;
	m_nMaxMemory = m_nAdded = 0;

	SetLimits(DEFAULT_MAX_MEM_KB, DEFAULT_TIMEOUT_SEC);
}

//------------------------------------------------------------------
// Called from storage when config is loaded.
//------------------------------------------------------------------
void CBlocksCache::SetLimits(unsigned long nMaxMemKB, unsigned int nTimeOutSec)
{
	if(nTimeOutSec < 1)
	{
		printf("CBlockCache. Timeout has invalid value '%d'. Forcing default value %d KB\n", nTimeOutSec, DEFAULT_TIMEOUT_SEC);
		nTimeOutSec = DEFAULT_TIMEOUT_SEC;
	}

	m_nTimeOutSec = nTimeOutSec;

	if(nMaxMemKB < MIN_MEM_KB)
	{
		printf("CBlockCache. Max memory reaches the max. Forcing default value %d KB\n", DEFAULT_MAX_MEM_KB);
		nMaxMemKB = DEFAULT_MAX_MEM_KB;
	}

	m_nMaxMemory = nMaxMemKB * 1024;
	// DEFAULT_PERCENT use 10% out of max values
	m_nMaxHandlesOpen = (GetMaxOpenFiles() / 100) * DEFAULT_PERCENT; // round to integer
	if(m_nMaxHandlesOpen < MIN_HANDLES_OPEN)
	{
		printf("BlocksCache. m_nMaxHandlesOpen has invalid value %d. Forcing default value %d.\n", m_nMaxHandlesOpen, MIN_HANDLES_OPEN);
		m_nMaxHandlesOpen = MIN_HANDLES_OPEN;
	}
	printf("BlocksCache. Setting limits. m_nMaxHandlesOpen=%i, m_nMaxMemory=%ld KB, timeout=%d sec\n", m_nMaxHandlesOpen, m_nMaxMemory / 1024, m_nTimeOutSec);
}

//------------------------------------------------------------------
//
//------------------------------------------------------------------
CBlocksCache::~CBlocksCache()
{
	Clean();
}

//------------------------------------------------------------------
//
//------------------------------------------------------------------
void CBlocksCache::Clean()
{
	CMutexGuard G(m_Mutex);
	if(m_mItems.size() == 0)
		return;

	TItems::iterator it = m_mItems.begin();
	for(; it != m_mItems.end(); ++it)
	{
		delete (*it).first;
		delete (*it).second;
	}
	m_mItems.clear();
}

//------------------------------------------------------------------
//
//------------------------------------------------------------------
CBlocksCacheItem* CBlocksCache::AddAndLock(const CZorgeString& strNum, CMsgBlock* pBlock)
{
	CMutexGuard G(m_Mutex);

	if(strNum.IsEmpty())
		throw CZorgeAssert("CStorageCache::Add. number is empty");
	if(pBlock == 0)
		throw CZorgeAssert("CStorageCache::Add. block is 0");

	if(++m_nAdded > g_nMaxAddedToCheckHandles)
	{
		m_nAdded = 0;
		ReleaseHandles();
	}

	CBlocksCacheItem* pItem = new CBlocksCacheItem(pBlock);
	m_mItems.insert(make_pair(new CZorgeString(strNum), pItem));
	pItem->Lock();

	return pItem;
}

//------------------------------------------------------------------
//
//------------------------------------------------------------------
void CBlocksCache::Delete(const CZorgeString& strNum)
{
	if(strNum.IsEmpty())
		throw CZorgeAssert("CStorageCache::Add. number is empty");

	time_t nStart = time(0);
	pthread_t nThread = pthread_self();
	bool bToBeDeleted = false;

	while(1)
	{
		// Scope for guard
		{
			CMutexGuard G(m_Mutex);
			TItems::iterator it = m_mItems.find(&strNum);
			if(it == m_mItems.end())
				return; // Block not found

			CBlocksCacheItem* pItem = (*it).second;
			if(!pItem->m_bLocked)
			{
				bToBeDeleted = true; // Block found and not locked.
			}
			else if(pItem->m_LockedBy == nThread)
			{
				bToBeDeleted = true; // Block found and locked by me
			}

			if(bToBeDeleted)
			{
				delete (*it).first;
				delete (*it).second;
				m_mItems.erase(it);
				return;
			}
			// Block locked, wait and try again or timeout
		}
		puts("CBlocksCache::Delete waiting");
		ZorgeSleep(100);
		time_t nTotal = time(0) - nStart;
		if(nTotal >= m_nTimeOutSec)
			throw CZorgeError("Blocks cache. Core is busy.");
	}
}

//------------------------------------------------------------------
//
//------------------------------------------------------------------
CBlocksCacheItem* CBlocksCache::Find(const CZorgeString* pStrNum)
{
	if(!pStrNum)
		throw CZorgeAssert("Assert(pStrNum) at CBlocksCache::Find");

	time_t nStart = time(0);
	pthread_t nThread = pthread_self();

	while(1)
	{
		// Scope for guard
		{
			CMutexGuard G(m_Mutex);
			TItems::iterator it = m_mItems.find(pStrNum);
			if(it == m_mItems.end())
				return 0;

			CBlocksCacheItem* pItem = (*it).second;
			if(!pItem->m_bLocked)
			{
				pItem->Lock();
				return pItem;
			}
			if(pItem->m_LockedBy == nThread)
				return pItem;

			// Block locked, wait and try again or timeout
		}
		printf("CBlocksCache::Find %lx waiting for %s\n", pthread_self(), (const char*)*pStrNum);
		ZorgeSleep(100);
		time_t nTotal = time(0) - nStart;
		if(nTotal >= m_nTimeOutSec)
			throw CZorgeError("Blocks cache. Core is busy.");
	}
	return 0;

}

//------------------------------------------------------------------
//
//------------------------------------------------------------------
void CBlocksCache::Print()
{
	unsigned int n = CMsgBlock::g_nTotalOpenHandlers;
	unsigned long nMem = CMsgBlock::g_nTotalMemory;
	printf("CBlocksCache, size()=%ld, g_nTotalOpenHandlers=%d, m_nMaxMemory=%ld KB, Memory used=%ld KB\n", m_mItems.size(), n, m_nMaxMemory/1024, nMem/1024);
/*
	TItems::iterator it = m_mItems.begin();
	for(; it != m_mItems.end(); ++it)
		(*it).second->Print();
	printf("<----- CBlocksCache");
*/
}

typedef pair<CBlocksCacheItem*, const CZorgeString*> T1;
static bool CompareItems(T1& p1, T1& p2)
{
	// ASC by last time access.
	return p1.first->m_LastAccess < p2.first->m_LastAccess;
}

// private
void CBlocksCache::ReleaseHandles()
{
	unsigned long nMemoryUsed = CMsgBlock::g_nTotalMemory;

	bool bReleaseMemory = nMemoryUsed >= m_nMaxMemory;

	unsigned int nOpenHandles = CMsgBlock::g_nTotalOpenHandlers;
	if(nOpenHandles < m_nMaxHandlesOpen)
		return; // nothing to do

	printf("CBlocksCache::ReleaseHandles() bReleaseMemory = %d\n", bReleaseMemory);

	typedef pair<CBlocksCacheItem*, const CZorgeString*> T1;
	typedef vector<T1> TSort;
	TSort vSort;

	CBlocksCacheItem* pItem = 0;
	const CZorgeString* strKey = 0;
	TItems::iterator it = m_mItems.begin();
	for(; it != m_mItems.end(); ++it)
	{
		pItem = (*it).second;
		strKey = (*it).first;
		if(!pItem->m_bLocked && pItem->GetBlock()->IsHandleOpen())
			vSort.push_back(make_pair(pItem, strKey));
	}

	TSort::iterator itV;
	sort(vSort.begin(), vSort.end(), CompareItems);

	// Close handles strategy is "half of open".
	unsigned int nLeft = nOpenHandles / 2;
	CMsgBlock* pBlock = 0;
	unsigned int nReleased = vSort.size();
	for(itV = vSort.begin(); itV != vSort.end(); ++itV, --nLeft)
	{
		if(nLeft == 0)
			break;

		strKey = (*itV).second;

		TItems::iterator itM = m_mItems.find(strKey);
		if(itM != m_mItems.end())
		{
			if(bReleaseMemory)
			{
				delete (*itM).first;
				delete (*itM).second;
				m_mItems.erase(itM);
			}
			else
			{
				pBlock = (*itM).second->m_pBlock;
				pBlock->CloseHandler();
			}
		}
		else
			printf("Not found in cache for release handles %s\n", (const char*)(*strKey));
	}
	printf("CBlocksCache::ReleaseHandles() released items = %d\n", nReleased);
}

//------------------------------------------------------------------
//
//------------------------------------------------------------------
void CBlocksCache::Diagnostics(CZorgeString& strOut)
{
	CMutexGuard G(m_Mutex);
	unsigned int n = CMsgBlock::g_nTotalOpenHandlers;
	unsigned long nMem = CMsgBlock::g_nTotalMemory;
	CZorgeString strTmp;
	strTmp.Format("CBlocksCache. size=%ld, OpenHandels=%d, MaxMemory=%ld KB, Memory used=%ld KB\n", m_mItems.size(), n, m_nMaxMemory/1024, nMem/1024);
	strOut += strTmp;
}

