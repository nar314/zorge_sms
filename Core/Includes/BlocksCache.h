// BlocksCache.h

#ifndef CORE_INCLUDES_BLOCKSCACHE_H_
#define CORE_INCLUDES_BLOCKSCACHE_H_

#include "MsgBlock.h"
#include "ZorgeMutex.h"

#include <map>
using namespace std;

//------------------------------------------------------------------
//
//------------------------------------------------------------------
class CBlocksCacheItem
{
public:
	bool		m_bLocked;
	time_t		m_LastAccess;
	pthread_t	m_LockedBy; // to allow re-entrance locking
	CMsgBlock*	m_pBlock;

	CBlocksCacheItem() = delete;
	CBlocksCacheItem(const CBlocksCacheItem&) = delete;
	CBlocksCacheItem& operator =(const CBlocksCacheItem&) = delete;

	CBlocksCacheItem(CMsgBlock*);

	~CBlocksCacheItem();
	CMsgBlock*	GetBlock();

	void		Lock();
	void		Release();
	void 		Print();
};

struct CBlockCacheItemGuard
{
	CBlockCacheItemGuard() = delete;
	CBlockCacheItemGuard(const CBlockCacheItemGuard&) = delete;
	CBlockCacheItemGuard& operator=(const CBlockCacheItemGuard&) = delete;

	CBlocksCacheItem* m_pItem = 0;
	CBlockCacheItemGuard(CBlocksCacheItem* p)
	{
		// there is a case when p is null.
		if(p && !p->m_bLocked)
			puts("Assert true CBlockCacheItemGuard");
		m_pItem = p; // p already locked by Find
	}
	void Set(CBlocksCacheItem* p)
	{
		if(!p->m_bLocked)
			puts("Assert true CBlockCacheItemGuard::Set");
		m_pItem = p; // p was just added to cache as locked.
	}
	~CBlockCacheItemGuard()
	{
		if(m_pItem)
			m_pItem->Release();
	}
};

typedef map<const CZorgeString*, CBlocksCacheItem*, CMapCmpPtr> TItems;

//------------------------------------------------------------------
//
//------------------------------------------------------------------
class CBlocksCache
{
	TItems				m_mItems;
	unsigned int  		m_nMaxHandlesOpen;
	unsigned long		m_nMaxMemory;
	CZorgeMutex			m_Mutex;
	unsigned long		m_nAdded;
	unsigned int 		m_nTimeOutSec;

	void 				ReleaseHandles();

public:
	CBlocksCache();
	~CBlocksCache();
	CBlocksCache(const CBlocksCache&) = delete;
	CBlocksCache& operator=(const CBlocksCache&) = delete;

	void 				SetLimits(unsigned long nMaxMemKB, unsigned int nTimeOutSec);
	CBlocksCacheItem*	AddAndLock(const CZorgeString& strNum, CMsgBlock* pBlock);
	void				Delete(const CZorgeString& strNum);
	CBlocksCacheItem* 	Find(const CZorgeString* pStrNum);
	void				Clean();
	void				Print();
	void				Diagnostics(CZorgeString& strOut);
};

#endif


