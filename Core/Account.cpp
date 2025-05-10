#include "Account.h"
#include "Globals.h"
#include "ZorgeError.h"
#include "CoderAES.h"
#include "SysUtils.h"
#include "StrUtils.h"
#include "CoderAESPool.h"

static const CZorgeString g_strConfig("config");
static const CZorgeString g_strMsgIn("msg.in");

#define CONFIG_ID 		"Id"
#define CONFIG_NAME 	"Name"
#define CONFIG_TOKEN 	"Token"

static CZorgeString g_strConfigEncr = "";
static CZorgeString g_strMsgInEncr = "";
static CZorgeString g_strTokenEncr = "";

std::atomic<unsigned int> g_CAccount = 0;
std::atomic<unsigned int> g_CAccount_max = 0;

struct CAccountCounter
{
	~CAccountCounter()
	{
		printf("g_CAccount = %d (%d)\n", (unsigned int)g_CAccount, (unsigned int)g_CAccount_max);
	}
	static void Add()
	{
		g_CAccount++;
		g_CAccount_max++;
	}
} g_CAccountCounter;

void DiagCAccount(CZorgeString& strOut)
{
	CZorgeString str;
	str.Format("CAccount.\tCur=%d, Max=%d\n", (unsigned int)g_CAccount, (unsigned int)g_CAccount_max);
	strOut += str;
}

// static
void CAccount::SetText(const CZorgeString& strRepoPin)
{
	CCoderPoolItem* pItem = g_RepoCoderPool.Get();
	CCoderPoolGuard Guard(pItem);
	CCoderAES* pCoder = pItem->m_pItem;

	if(pCoder->EncryptToHexChars(g_strConfig, g_strConfigEncr) != CODER_OK)
		throw CZorgeAssert("Failed to encrypt account config file name.");

	if(pCoder->EncryptToHexChars(g_strMsgIn, g_strMsgInEncr) != CODER_OK)
		throw CZorgeAssert("Failed to encrypt account input folder file name.");
}

//--------------------------------------------------------------------
// Called from flow of adding new number
//--------------------------------------------------------------------
CAccount::CAccount(const CZorgeString& strCleanNumber, const CZorgeString& strRoot, const CZorgeString& strPin)
{
	CAccountCounter::Add();

	if(strCleanNumber.IsEmpty())
		throw CZorgeError("Number is empty");

	if(strRoot.IsEmpty())
		throw CZorgeError("Root is empty");

	m_strPin = strPin;
	m_strCleanNum = strCleanNumber;
	m_strRoot = strRoot;
	if(m_strRoot.GetLastChar() != '/')
		m_strRoot += '/';

	m_strInFolderPath = m_strRoot;
	m_strInFolderPath += g_strMsgInEncr;
	m_strInFolderPath += "/";

	CCoderPoolItem* pItem = g_RepoCoderPool.Get();
	CCoderPoolGuard Guard(pItem);
	CCoderAES* pCoderRepo = pItem->m_pItem;

	// Generate default config
	CZorgeString strValue, strValueEnc;

	CCoderAES CoderNum;
	CoderNum.Init(m_strPin);
	GuidStr(strValue);
	if(!CoderNum.EncryptToHexChars(strValue, strValueEnc))
		throw CZorgeAssert("Failed to encrypt account config file id.");

	m_Config.SetValue(CONFIG_ID, strValueEnc);
	strValue = "";
	m_Config.SetValue(CONFIG_NAME, strValue);
	m_strToken.EmptyData(); // account has not token by default.
	m_Config.SetValue(CONFIG_TOKEN, m_strToken);

	CZorgeString strConfigContent, strConfigContentEncr;
	m_Config.ToString(strConfigContent);

	CMemBuffer Buf(strConfigContent), BufEncr;
	if(pCoderRepo->Encrypt(Buf, BufEncr) != CODER_OK)
		throw CZorgeAssert("Failed to encrypt account config file data.");

	strValue = m_strRoot;
	strValue += g_strConfigEncr;
	if(!MemToFile(strValue, BufEncr))
		throw CZorgeAssert("Failed to store config.");
}

//--------------------------------------------------------------------
// Flow of loading account.
//--------------------------------------------------------------------
CAccount::CAccount(const CZorgeString& strCleanNumber, const CZorgeString& strRoot)
{
	CAccountCounter::Add();

	if(strCleanNumber.IsEmpty())
		throw CZorgeError("Number is empty");

	if(strRoot.IsEmpty())
		throw CZorgeError("Root is empty");

	m_strToken.EmptyData();
	m_strCleanNum = strCleanNumber;
	m_strRoot = strRoot;
	if(m_strRoot.GetLastChar() != '/')
		m_strRoot += '/';

	m_strInFolderPath = m_strRoot;
	m_strInFolderPath += g_strMsgInEncr;
	m_strInFolderPath += "/";

	CCoderPoolItem* pItem = g_RepoCoderPool.Get();
	CCoderPoolGuard Guard(pItem);
	CCoderAES* pCoderRepo = pItem->m_pItem;

	CZorgeString strPath(m_strRoot);
	strPath += g_strConfigEncr;

	CMemBuffer BufEncr, BufDecr;
	if(!FileToMem(strPath, BufEncr))
		throw CZorgeAssert("Failed to load config.");

	if(pCoderRepo->Decrypt(BufEncr, BufDecr) != CODER_OK)
		throw CZorgeAssert("Failed to decrypt account config file data.");

	CZorgeString strConfigDecr;
	BufDecr.ToString(strConfigDecr);

	m_Config.FromString(strConfigDecr);
	m_Config.GetValue(CONFIG_TOKEN, m_strToken);
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CAccount::~CAccount()
{
	g_CAccount--;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
const CZorgeString& CAccount::GetNum() const
{
	return m_strCleanNum;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CAccount::Print()
{
	printf("%s, root = %s, pin=%s, folder=%s\n", (const char*)m_strCleanNum, (const char*)m_strRoot, (const char*)m_strPin, (const char*)m_strInFolderPath);
}

// private
void CAccount::CheckPin(const CZorgeString& strPin, bool bSendMsg)
{
	if(strPin.IsEmpty())
		throw CZorgeError("Pin can not be empty.");

	if(bSendMsg && strPin == m_strToken)
		return; // Token is OK for call to send message

	if(m_strPin.IsEmpty())
	{
		CZorgeString strEncrId;
		if(!m_Config.GetValue(CONFIG_ID, strEncrId))
			throw CZorgeAssert("Id not found in config.");

		m_Config.GetValue(CONFIG_TOKEN, m_strToken);
		if(bSendMsg && strPin == m_strToken)
			return; // Token is OK for call to send message

		CCoderAES Coder;
		Coder.Init(strPin);

		CZorgeString strDecrId;
		if(!Coder.DecryptFromHexChars(strEncrId, strDecrId))
			throw CZorgeError("Invalid pin");

		m_strPin = strPin;
		return; // Pin is OK
	}
	else if(strPin == m_strPin)
		return; // Pin is OK

	throw CZorgeError("Invalid pin");
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CAccount::Delete(const CZorgeString& strPin, TLines& vOut)
{
	vOut.clear();
	CheckPin(strPin, false);

	// Collect lines to clean up blocks cache
	vOut.push_back(m_strCleanNum);

	CZorgeString strNumRoot(m_strRoot), strPathToConfig;

	// Delete all devices and config.
	TFsNames vItems;
	int nRc(0);
	CZorgeString strFullPathItem;
	EnumFolder(strNumRoot, vItems);
	for(TFsNames::const_iterator it = vItems.begin(); it != vItems.end(); ++it)
	{
		const CFsName& Name = *it;

		strFullPathItem = strNumRoot;
		strFullPathItem += Name.m_strName;
		if(!Name.m_bFolder)
		{
			nRc = Unlink(strFullPathItem);
			if(nRc != 0)
			{
				CZorgeString strDeleted(strNumRoot);
				strDeleted += "deleted_";
				strDeleted += Name.m_strName;
				nRc = Rename(strFullPathItem, strDeleted);
				if(nRc != 0)
					puts("Failed to rename");
			}
		}
	}

	try
	{
		DeleteFilesTree((const char*)m_strInFolderPath, 0);
		nRc = RmDir(m_strInFolderPath);
		if(nRc != 0)
			throw CZorgeAssert("Failed to delete");
	}
	catch(CZorgeError& e)
	{
		CZorgeString strDeleted(strNumRoot);
		strDeleted += "deleted_";
		strDeleted += m_strInFolderPath;
		nRc = Rename(strFullPathItem, strDeleted);
		if(nRc != 0)
			puts("Failed to rename");
	}
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CAccount::Open(const CZorgeString& strPin)
{
	CheckPin(strPin, false);

	// Check if input folder exist.
	if(IsFileExist((const char*)m_strInFolderPath) != FOLDER_EXIST)
		throw CZorgeError("Folder does not exist.");
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CAccount::GetMsgCount(CBlocksCache& BlocksCache, const CZorgeString& strPin,
							unsigned int& nTotal, unsigned int& nNew)
{
	CheckPin(strPin, false);

	CBlocksCacheItem* pItem = BlocksCache.Find(&m_strCleanNum);
	CBlockCacheItemGuard Guard(pItem);
	CMsgBlock* pMsgBlock = 0;
	if(pItem == 0)
	{
		CZorgeString strFolderRoot(m_strRoot);
		strFolderRoot += g_strMsgInEncr;

		pMsgBlock = new CMsgBlock(strFolderRoot);
		CBlocksCacheItem* p = BlocksCache.AddAndLock(m_strCleanNum, pMsgBlock);
		Guard.Set(p);
	}
	else
		pMsgBlock = pItem->GetBlock();

	pMsgBlock->GetCount(nTotal, nNew);
}

// private
void CAccount::StoreMsg(CBlocksCache& BlocksCache, const CZorgeString& strFromNum, const CZorgeString& strMsg, CZorgeString& strMsgGuid)
{
	CZorgeString strFilePath;
	if(!GuidStr(strMsgGuid))
		throw CZorgeAssert("Failed to generate GUID.");

	CCoderPoolItem* pItem = g_RepoCoderPool.Get();
	CCoderPoolGuard Guard(pItem);
	CCoderAES* pCoderRepo = pItem->m_pItem;

	strFilePath = m_strInFolderPath;
	strFilePath += strMsgGuid;

	CMemBuffer Buf(strMsg), BufEncr;
	if(pCoderRepo->Encrypt(Buf, BufEncr) != CODER_OK)
		throw CZorgeAssert("Failed to encrypt message.");

	if(!MemToFile(strFilePath, BufEncr))
		throw CZorgeAssert("Failed to store message.");

	{
		CBlocksCacheItem* pItem = BlocksCache.Find(&m_strCleanNum);
		CBlockCacheItemGuard Guard(pItem);
		CMsgBlock* pMsgBlock = 0;
		if(pItem == 0)
		{
			pMsgBlock = new CMsgBlock(m_strInFolderPath);
			CBlocksCacheItem* p = BlocksCache.AddAndLock(m_strCleanNum, pMsgBlock);
			Guard.Set(p);
		}
		else
			pMsgBlock = pItem->GetBlock();

		try
		{
			pMsgBlock->Add(strFromNum, strMsgGuid);
		}
		catch(CZorgeError& e)
		{
			if(PurgeFile(strFilePath) != 0)
				printf("Failed to delete file %s\n", (const char*)strFilePath);
			throw;
		}
	}
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CAccount::SendMsg(CBlocksCache& BlocksCache,
		const CZorgeString& strFromPin, const CZorgeString& strFromCleanNum, const CZorgeString& strMsg,
		CZorgeString& strMsgGuid)
{
	// Deliver message
	StoreMsg(BlocksCache, strFromCleanNum, strMsg, strMsgGuid);
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CAccount::GetIds(CBlocksCache& BlocksCache, const CZorgeString& strPin, unsigned int nType, TIds& vIds)
{
	CheckPin(strPin, false);

	CBlocksCacheItem* pItem = BlocksCache.Find(&m_strCleanNum);
	CBlockCacheItemGuard Guard(pItem);
	CMsgBlock* pMsgBlock = 0;
	if(pItem == 0)
	{
		pMsgBlock = new CMsgBlock(m_strInFolderPath);
		CBlocksCacheItem* p = BlocksCache.AddAndLock(m_strCleanNum, pMsgBlock);
		Guard.Set(p);
	}
	else
		pMsgBlock = pItem->GetBlock();

	pMsgBlock->GetIds(nType, vIds);
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CMessage* CAccount::ReadMessage(CBlocksCache& BlocksCache, const CZorgeString& strPin, const CZorgeString& strMsgId)
{
	CheckPin(strPin, false);

	CBlocksCacheItem* pItem = BlocksCache.Find(&m_strCleanNum);
	CBlockCacheItemGuard Guard(pItem);
	CMsgBlock* pMsgBlock = 0;
	if(pItem == 0)
	{
		pMsgBlock = new CMsgBlock(m_strInFolderPath);
		CBlocksCacheItem* p = BlocksCache.AddAndLock(m_strCleanNum, pMsgBlock);
		Guard.Set(p);
	}
	else
		pMsgBlock = pItem->GetBlock();

	CMessage* pOut = pMsgBlock->ReadMessage(strMsgId);
	return pOut;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
int CAccount::DeleteMessage(CBlocksCache& BlocksCache, const CZorgeString& strPin, const CZorgeString& strMsgId)
{
	CheckPin(strPin, false);

	CBlocksCacheItem* pItem = BlocksCache.Find(&m_strCleanNum);
	CBlockCacheItemGuard Guard(pItem);
	CMsgBlock* pMsgBlock = 0;
	if(pItem == 0)
	{
		pMsgBlock = new CMsgBlock(m_strInFolderPath);
		CBlocksCacheItem* p = BlocksCache.AddAndLock(m_strCleanNum, pMsgBlock);
		Guard.Set(p);
	}
	else
		pMsgBlock = pItem->GetBlock();

	return pMsgBlock->DeleteMessage(strMsgId);
}

//--------------------------------------------------------------------
// static
//--------------------------------------------------------------------
bool CAccount::IsAccountFileExist(const CZorgeString& strAccRoot)
{
	CZorgeString strFullPath(strAccRoot);
	strFullPath += g_strConfigEncr;

	return IsFileExist((const char*)strFullPath) == FILE_EXIST;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CAccount::CreateInFolder(const CZorgeString& strAccPin)
{
	CheckPin(strAccPin, false);
	if(m_strPin.IsEmpty())
		throw CZorgeError("No pin.");

	// Create "in folder".
	if(IsFileExist((const char*)m_strInFolderPath) == FOLDER_EXIST)
		throw CZorgeError("Folder already exist.");

	if(MkDir(m_strInFolderPath) != 0)
		throw CZorgeError("Failed to create folder.");
}

//--------------------------------------------------------------------
// Change pin
//--------------------------------------------------------------------
void CAccount::ChangePin(const CZorgeString& strPin, const CZorgeString& strNewPin)
{
	CheckPin(strPin, false);

	if(m_strPin == strNewPin)
		return;

	m_strPin = strNewPin;
	CZorgeString strValue, strValueEnc;

	CCoderPoolItem* pItem = g_RepoCoderPool.Get();
	CCoderPoolGuard Guard(pItem);
	CCoderAES* pCoderRepo = pItem->m_pItem;

	CCoderAES CoderNum;
	CoderNum.Init(m_strPin);
	GuidStr(strValue);
	if(!CoderNum.EncryptToHexChars(strValue, strValueEnc))
		throw CZorgeAssert("Failed to encrypt account config file id.");

	m_Config.SetValue(CONFIG_ID, strValueEnc);
	strValue = "";
	m_Config.SetValue(CONFIG_NAME, strValue);

	CZorgeString strConfigContent, strConfigContentEncr;
	m_Config.ToString(strConfigContent);

	CMemBuffer Buf(strConfigContent), BufEncr;
	if(pCoderRepo->Encrypt(Buf, BufEncr) != CODER_OK)
		throw CZorgeAssert("Failed to encrypt account config file data.");

	strValue = m_strRoot;
	strValue += g_strConfigEncr;
	if(!MemToFile(strValue, BufEncr))
		throw CZorgeAssert("Failed to store config.");
}

//--------------------------------------------------------------------
// Get token.
//--------------------------------------------------------------------
const CZorgeString& CAccount::GetToken() const
{
	return m_strToken;
}

// private
void CAccount::StoreToken(CZorgeString& strToken)
{
	m_Config.SetValue(CONFIG_TOKEN, strToken);

	CZorgeString strConfigContent, strConfigContentEncr;
	m_Config.ToString(strConfigContent);

	CCoderPoolItem* pItem = g_RepoCoderPool.Get();
	CCoderPoolGuard Guard(pItem);
	CCoderAES* pCoderRepo = pItem->m_pItem;

	CMemBuffer Buf(strConfigContent), BufEncr;
	if(pCoderRepo->Encrypt(Buf, BufEncr) != CODER_OK)
		throw CZorgeAssert("Failed to encrypt account config file data.");

	CZorgeString strPath(m_strRoot);
	strPath += g_strConfigEncr;
	if(!MemToFile(strPath, BufEncr))
		throw CZorgeAssert("Failed to store config.");
}

//--------------------------------------------------------------------
// Generate new token.
//--------------------------------------------------------------------
const CZorgeString& CAccount::NewToken()
{
	GuidStr(m_strToken);
	StoreToken(m_strToken);
	return m_strToken;
}

//--------------------------------------------------------------------
// Delete token.
//--------------------------------------------------------------------
void CAccount::DeleteToken()
{
	m_strToken.Empty();
	StoreToken(m_strToken);
}

//--------------------------------------------------------------------
// Check token.
//--------------------------------------------------------------------
bool CAccount::CheckToken(const CZorgeString& strToken)
{
	if(strToken.IsEmpty())
		return false;

	return m_strToken == strToken;
}
