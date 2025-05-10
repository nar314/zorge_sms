// OpenConsCache.h

#ifndef OPEN_CONS_CACHE_H_
#define OPEN_CONS_CACHE_H_

#include <time.h>
#include <map>
#include "ZorgeString.h"
#include "ZorgeMutex.h"

#define GUID_LEN 33 // (16 * 2) + null terminator.

enum CacheItemStatus
{
	InProcessing = 0,
	Done,
	NotFound
};

struct COpenConsItem
{
	char			m_szClientId[GUID_LEN];
	unsigned int	m_nSeqNum;
	time_t 			m_nLastAccess;
	CZorgeString	m_strReply;
	bool			m_bError;
	CacheItemStatus	m_Status;

	COpenConsItem(const COpenConsItem&) = delete;
	COpenConsItem();
	~COpenConsItem();

	void Print()
	{
		char szTmp[33];
		memcpy(szTmp, m_szClientId, 32);
		szTmp[32] = 0;
		printf("%s, %X, status=%d, time=%ld, bError=%d, value=%s\n", szTmp, m_nSeqNum, m_Status, m_nLastAccess, m_bError, (const char*)m_strReply);
	}
};

#define CLIENT_ID_LEN_BYTES 32
struct COpenConsItemCompare
{
    bool operator()(const char* p1, const char* p2) const
    {
    	return memcmp(p1, p2, CLIENT_ID_LEN_BYTES) < 0;
    }
};

typedef map<const char*, COpenConsItem*, COpenConsItemCompare> TOpenItems;

//-------------------------------------------------------------
//
//-------------------------------------------------------------
class COpenConsCache
{
	CZorgeMutex		m_Mutex;
	TOpenItems		m_Map;
	time_t			m_nMaxLiveSec; // items older than m_nMaxLiveSec will be expired.
	time_t			m_nExpiredCalled;
	COpenConsItem* 	m_pOldestItem;
	bool			m_bTrace;

	void			FindOldestItem();
	void 			Print();
	void			RemoveExpired(const char* szCaller);

public:
	COpenConsCache();
	COpenConsCache(const COpenConsCache&) = delete;
	~COpenConsCache();

	// Add called when request accepted, but before processing it.
	void	Add(const char* szClientId, unsigned int nSeqNum);

	// Update called when request processed.
	void	Update(const char* szClientId, unsigned int nSeqNum, bool bError, const CZorgeString& strReply);

	// Find called when client asking for reply second time.
	CacheItemStatus	Find(const char* szClientId, unsigned int nSeqNum, bool& bError, CZorgeString& strReply);

	void	Diagnostics(CZorgeString& strOut);

	void	SetTrace(bool b);
	void	SetTTL(unsigned int nSec);
};

#endif
