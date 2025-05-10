// AccountCache.h
#ifndef CORE_INCLUDES_ACCOUNTCACHE_H_
#define CORE_INCLUDES_ACCOUNTCACHE_H_

#include <map>
using namespace std;

#include "Account.h"
#include "ZorgeMutex.h"

class CAccountCacheItem
{
public:
	bool		m_bLocked;
	time_t		m_LastAccess;
	pthread_t	m_LockedBy; // to allow re-entrance locking
	CAccount*	m_pAccount;

	CAccountCacheItem(const CAccountCacheItem&) = delete;
	CAccountCacheItem& operator =(const CAccountCacheItem&) = delete;

	CAccountCacheItem(CAccount*);
	~CAccountCacheItem();

	CAccount*	GetAccount();

	void		Lock();
	void		Release();
	void 		Print();
};

struct CAccountCacheItemGuard
{
	CAccountCacheItemGuard() = delete;
	CAccountCacheItemGuard(const CAccountCacheItemGuard&) = delete;
	CAccountCacheItemGuard& operator=(const CAccountCacheItemGuard&) = delete;

	CAccountCacheItem* m_pItem = 0;
	CAccountCacheItemGuard(CAccountCacheItem* p)
	{
		if(!p->m_bLocked)
			puts("Assert true at CAccountCacheItemGuard");
		m_pItem = p; // p already locked by Find
	}
	void Set(CAccountCacheItem* p)
	{
		m_pItem = p; // p was just added to cache as locked.
	}
	~CAccountCacheItemGuard()
	{
		if(m_pItem)
			m_pItem->Release();
	}
};

typedef map<const CZorgeString*, CAccountCacheItem*, CMapCmpPtr> TAccounts;

//----------------------------------------------------------------
//
//----------------------------------------------------------------
class CAccountCache
{
	CZorgeMutex			m_Lock;
	TAccounts			m_mAccounts;
	unsigned long		m_nMaxMemory;
	unsigned long		m_nMaxItems;
	unsigned long		m_nDeleted;
	unsigned int		m_nTimeOutSec;

	void				Release();
public:
	CAccountCache(const CAccountCache&) = delete;
	CAccountCache operator=(const CAccountCache&) = delete;

	CAccountCache();
	~CAccountCache();

	void 				SetLimits(unsigned long nMaxMemKB, unsigned int nTimeOutSec);
	CAccountCacheItem*	AddAndLock(CAccount* pAccount);
	void				Delete(const CZorgeString* pNum);

	CAccountCacheItem*	Find(const CZorgeString* pNum);
	void				Clean();
	void 				Print();
	void				Diagnostics(CZorgeString& strOut);
};

#endif
