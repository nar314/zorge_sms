// Account.h

#ifndef CORE_INCLUDES_ACCOUNT_H_
#define CORE_INCLUDES_ACCOUNT_H_

#include "BlocksCache.h"
#include "Config.h"

typedef pair<CZorgeString*, CZorgeString*> TPair;
typedef vector<CZorgeString> TLines;

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
class CAccount
{
	CZorgeString		m_strCleanNum;
	CZorgeString		m_strRoot;
	CZorgeString		m_strPin;
	CConfig				m_Config;
	CZorgeString		m_strInFolderPath;
	CZorgeString		m_strToken;

	void				StoreMsg(CBlocksCache& BlocksCache, const CZorgeString& strFromNum, const CZorgeString& strMsg, CZorgeString& strMsgGuid);
	void 				StoreToken(CZorgeString& strToken);

public:
	CAccount(const CAccount&) = delete;
	CAccount operator=(const CAccount&) = delete;

	CAccount(const CZorgeString& strCleanNumber, const CZorgeString& strRoot);
	CAccount(const CZorgeString& strCleanNumber, const CZorgeString& strRoot, const CZorgeString& strPin);
	~CAccount();

	const CZorgeString& GetNum() const;

	void				Delete(const CZorgeString&, TLines&);
	void				Open(const CZorgeString& strPin);

	void				CheckPin(const CZorgeString& strPin, bool bSendMsg);
	void 				GetMsgCount(CBlocksCache& BlocksCache, const CZorgeString& strPin, unsigned int& nTotal, unsigned int& nNew);
	void				SendMsg(CBlocksCache& BlocksCache,const CZorgeString& strFromPin, const CZorgeString& strFromCleanNum, const CZorgeString& strMsg, CZorgeString& strMsgGuid);
	void 				GetIds(CBlocksCache& BlocksCache, const CZorgeString& strPin, unsigned int nType, TIds& vIds);
	CMessage* 			ReadMessage(CBlocksCache& BlocksCache, const CZorgeString& strPin, const CZorgeString& strMsgId);
	int 				DeleteMessage(CBlocksCache& BlocksCache, const CZorgeString& strPin, const CZorgeString& strMsgId);
	void 				CreateInFolder(const CZorgeString& strAccPin);
	void				ChangePin(const CZorgeString& strPin, const CZorgeString& strNewPin);

	const CZorgeString& GetToken() const;
	const CZorgeString& NewToken();
	void 				DeleteToken();
	bool 				CheckToken(const CZorgeString& strToken);

	void Print();
#define SetPsw SetText
	static void			SetText(const CZorgeString& strRepoPin);
	static bool 		IsAccountFileExist(const CZorgeString& strAccRoot);
};

#endif
