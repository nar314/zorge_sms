// Config.h

#ifndef CORE_CONFIG_H_
#define CORE_CONFIG_H_

#include "ZorgeString.h"
#include <map>

using namespace std;

typedef map<CZorgeString, CZorgeString, CMapCmp> TMap;
typedef vector<pair<CZorgeString, CZorgeString>> TValues;

#define CONFIG_REPO_ID 				"Repo.Id"
#define CONFIG_REPO_KEY				"Repo.Key"
// UDP server
#define CONFIG_LISTENER_PORT 		"Listener.Port"
#define CONFIG_THREAD_POOL_SIZE		"Thread.Pool.Size"
#define CONFIG_THREAD_POOL_MONITOR_TIMEOUT	"Thread.Pool.Monitor.Timeout"
#define CONFIG_CODER_POOL_MAX 		"Coder.Pool.Max"
#define CONFIG_BLOCKS_CACHE_MEM_KB 	"Blocks.Cache.Mem.KB"
#define CONFIG_ACC_CACHE_MEM_KB 	"Acc.Cache.Mem.KB"
#define CONFIG_ACC_CACHE_TIMEOUT	"Acc.Cache.Timeout"
#define CONFIG_BLOCKS_CACHE_TIMEOUT	"Blocks.Cache.Timeout"
#define CONFIG_REPLY_TRACE 			"Reply.Trace"
#define CONFIG_REPLY_CHACHE_TTL 	"Reply.Cache.TTL"


//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
class CConfig
{
	TMap	m_mComments;
	TValues	m_mValues;

public:
	CConfig(const CConfig&) = delete;
	CConfig& operator=(const CConfig&) = delete;

	CConfig();
	~CConfig();

	void	Clear();
	int 	FromString(const CZorgeString&);
	void	ToString(CZorgeString&);

	bool	GetValue(CZorgeString&, CZorgeString&) const;
	bool	SetValue(CZorgeString&, CZorgeString&);

	bool	SetComment(const char*, const char*);

	bool	SetValue(const char*, CZorgeString&);
	bool	GetValue(const char*, CZorgeString&) const;

	void 	Print();
};

CConfig& GetConfig();

#endif
