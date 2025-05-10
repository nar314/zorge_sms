// CoderAESPoll.h

#ifndef CORE_CODERAESPOOL_H_
#define CORE_CODERAESPOOL_H_

#include "CoderAES.h"
#include "ZorgeMutex.h"

#define CODER_AES_POLL_MAX 64
#define CODER_AES_POLL_MIN 2

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
class CCoderPoolItem
{
public:
	CCoderAES* 	m_pItem = nullptr;
	bool		m_bLocked = false;
	bool		m_bFreeOnRelease = false;

	CCoderPoolItem(const CCoderPoolItem&) = delete;
	CCoderPoolItem operator=(const CCoderPoolItem&) = delete;

	CCoderPoolItem(CCoderAES*);
	~CCoderPoolItem();

	void 		Lock();
	void 		Release();
	void		Print();
};

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
struct CCoderPoolGuard
{
	CCoderPoolItem* m_pItem = nullptr;

	CCoderPoolGuard(const CCoderPoolGuard&) = delete;
	CCoderPoolGuard operator=(const CCoderPoolGuard&) = delete;

	CCoderPoolGuard(CCoderPoolItem* p)
	{
		m_pItem = p; // p locked by pool
	}

	~CCoderPoolGuard()
	{
		if(m_pItem)
			m_pItem->Release();
	}
};

typedef vector<CCoderPoolItem*> TCoders;

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
class CCoderAESPool
{
	size_t 			m_nMaxSize = CODER_AES_POLL_MAX;
	TCoders			m_vCoders;
	CZorgeString	m_strPsw;
	CZorgeMutex		m_Lock;

public:
	CCoderAESPool();
	~CCoderAESPool();

	CCoderAESPool(const CCoderAESPool&) = delete;
	CCoderAESPool operator=(const CCoderAESPool&) = delete;

	void 			Set(size_t, const CZorgeString&);
	CCoderPoolItem*	Get();
	void			Print();
	void			Clear();
};

extern CCoderAESPool g_RepoCoderPool;

#endif
