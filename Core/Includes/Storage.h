// Storage.h

#ifndef CORE_INCLUDES_STORAGE_H_
#define CORE_INCLUDES_STORAGE_H_

#include "AccountCache.h"
#include "BlocksCache.h"
#include "CoderAES.h"
#include "Config.h"
#include "AccountLocker.h"

#define TOTAL_BYTES 255

struct CNumDev
{
	CZorgeString	m_strNum;
	CZorgeString	m_strPin;
};

typedef vector<CZorgeString> TStrings;

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
class CStorage
{
public:
	enum Status
	{
		Unknown = 0,
		ReadyToOpen,
		Open,
		NotInit,
		Error
	};

private:
	CZorgeString		m_strRoot;
	CZorgeString		m_strKey;
	CZorgeString		m_strId;
	char*				m_arLetter[TOTAL_BYTES + 1]; // I need 0..255 inclusive
	CBlocksCache		m_BlocksCache;
	CAccountCache		m_AccountsCache;
	Status				m_Status;
	CCoderAES			m_Coder;
	CConfig&			m_Config;
	CAccountLocker		m_Locker;

	void 				InitBytesArray();
	void 				GetDigits(const CZorgeString& strSource, CZorgeString& strOut, TStrings& vOut, CZorgeString& strCleanNum);
	void	 			GetCleanNumWithPath(const CZorgeString& strSource, CZorgeString& strPath, CZorgeString& strCleanNum);
	void 				GetCleanNum(const CZorgeString& strSource, CZorgeString& strCleanNum);
	CAccountCacheItem*	FindAccount(const CZorgeString& strNum);
	void				GenerateConfigFile();

public:
	CStorage();
	~CStorage();
						CStorage(const CStorage&) = delete;
	CStorage& 			operator=(const CStorage&) = delete;

	int					SetRoot(const CZorgeString& strRoot);
	int 				Login(const CZorgeString& strKey);
	void				Init(const CZorgeString& strKey);
	Status				GetStatus();
	const char*			GetStatusStr();
	static const char* 	GetStatusStr(const Status& Status);
	void				Diagnostics(CZorgeString& strOut);

	void				AddNumber(const CZorgeString& strNewNum, const CZorgeString& strPin, CZorgeString& strCreatedNumber);
	bool				FindNumber(const CZorgeString& strNum);
	void				OpenNumber(const CZorgeString& strNum, const CZorgeString& strPin);
	void 				DeleteNumber(const CZorgeString& strNum, const CZorgeString& strPin);

	void 				GetMsgCount(const CNumDev& From, unsigned int& nTotal, unsigned int& nNew);
	void 				SendMsg(const CNumDev& From, const CZorgeString& strToNum, const CZorgeString& strMsg, CZorgeString& strMsgGuid);
	void 				GetIds(const CNumDev& From, unsigned int nType, TIds& vIds);
	CMessage* 			ReadMessage(const CNumDev& From, const CZorgeString& strMsgId);
	int 				DeleteMessage(const CNumDev& From, const CZorgeString& strMsgId);
	void 				ChangePin(const CNumDev& Acc, const CZorgeString& strNewPin);

	const CZorgeString& GetNumToken(const CNumDev& Num);
	const CZorgeString& NewNumToken(const CNumDev& Num);
	void 				DeleteNumToken(const CNumDev& Num);
	bool 				CheckNumToken(const CNumDev& Num);
};

#endif
