#include "Storage.h"
#include <stdlib.h> // for atol
#include <unistd.h> // for geteuid

#include "Globals.h"

#include "SysUtils.h"
#include "StrUtils.h"
#include "MsgBlock.h"
#include "CoderAESPool.h"
#include "CoderBF.h"
#include "Diag.h"

static CZorgeString g_strConfigEncr = "";

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
CStorage::CStorage() : m_Config(GetConfig())
{
	m_Status = Unknown;
	memset(m_arLetter, 0, sizeof(m_arLetter));
	uid_t nId = geteuid();
	if(nId == 0)
		puts("Hey, you, why I am running as a root ?");
	if(nId != getuid())
		puts("Something wrong, I am running not as myself.");
}

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
CStorage::~CStorage()
{
	for(int n = 0; n < TOTAL_BYTES; ++n)
		delete [] m_arLetter[n];
}

//-------------------------------------------------------------------
// strRoot = "/home/dkhaliap/tmp/tz_storage"
//-------------------------------------------------------------------
int CStorage::SetRoot(const CZorgeString& strRoot)
{
	if(strRoot.IsEmpty())
		throw CZorgeError("Storage root is empty.");

	m_strRoot = strRoot;
	m_strRoot.Trim();
	if(m_strRoot.GetLastChar() != '/')
		m_strRoot += "/";

	// 1. Is storage root folder exist ?
	int nRc = IsFileExist((const char*)strRoot);
	if(nRc != FOLDER_EXIST)
	{
		CZorgeString strMsg("Storage root not found. Folder : ");
		strMsg += strRoot;
		printf("%s\n", (const char*)strMsg);
		m_Status = NotInit;
		return m_Status;
	}

	// Root folder exist.
	CZorgeString strConfigPath(m_strRoot);
	strConfigPath += "config";
	m_Status = IsFileExist(strConfigPath) == FILE_EXIST ? ReadyToOpen : NotInit;
	return m_Status;
}

//-------------------------------------------------------------------
// strRoot = "/home/dkhaliap/tmp/tz_storage"
//-------------------------------------------------------------------
int CStorage::Login(const CZorgeString& strKey)
{
	if(m_strRoot.IsEmpty())
		throw CZorgeError("Root is empty.");

	if(strKey.IsEmpty())
		throw CZorgeError("Key is empty.");

	if(m_Status != ReadyToOpen)
	{
		if(SetRoot(m_strRoot) != ReadyToOpen)
			throw CZorgeError("Storage root does not exist or storage not initialized.");
	}

	m_Coder.Init(strKey);
	m_strKey = strKey;
	g_RepoCoderPool.Set(CODER_AES_POLL_MAX, m_strKey);

	// 2. Is storage config file exist ?
	CZorgeString strPath;
	if(m_Coder.EncryptToHexChars("config", g_strConfigEncr) != CODER_OK)
		throw CZorgeAssert("Failed to encrypt");

	strPath = m_strRoot;
	strPath += "config";

	int nRc = IsFileExist((const char*)strPath);
	if(nRc != FILE_EXIST)
	{
		if(IsFolderEmpty((const char*)m_strRoot))
		{
			m_Status = NotInit;
			throw CZorgeError("Storage not initialized.");
		}
		else
		{
			m_Status = Error;
			throw CZorgeError("Repository password is invalid.");
		}
	}

	CZorgeString strConfigData;
	if(!FileToString(strPath, strConfigData))
		throw CZorgeAssert("Failed to load config.");

	CZorgeString strRepoKey, strValue;
	m_Config.FromString(strConfigData);
	if(!m_Config.GetValue(CONFIG_REPO_KEY, strRepoKey))
		throw CZorgeError("Repository key not found in config.");
	if(!m_Config.GetValue(CONFIG_REPO_ID, m_strId))
		throw CZorgeError("Repository id not found in config.");

	CZorgeString strDecrKey;
	if(m_Coder.DecryptFromHexChars(strRepoKey, strDecrKey) != CODER_OK)
		throw CZorgeError("Invalid key");

	if(strDecrKey != m_strId)
		throw CZorgeError("Invalid key. 2");

	// 4. Login succeed.
	InitBytesArray();

	m_Status = Open;

	if(!m_Config.GetValue(CONFIG_BLOCKS_CACHE_MEM_KB, strValue))
		throw CZorgeError("Blocks.Cache.Mem.KB not found in config.");
	unsigned long nBlolocksMemKB = atol((const char*)strValue);

	if(!m_Config.GetValue(CONFIG_BLOCKS_CACHE_TIMEOUT, strValue))
		throw CZorgeError("Blocks.Cache.Timeout not found in config.");
	unsigned int nBlocksTimeout = atoi((const char*)strValue);

	if(!m_Config.GetValue(CONFIG_ACC_CACHE_MEM_KB, strValue))
		throw CZorgeError("Acc.Cache.Mem.KB not found in config.");
	unsigned long nAccMemKB = atol((const char*)strValue);

	if(!m_Config.GetValue(CONFIG_ACC_CACHE_TIMEOUT, strValue))
		throw CZorgeError("Acc.Cache.Timeout not found in config.");
	unsigned int nAccTimeout = atoi((const char*)strValue);

	m_AccountsCache.Clean();
	m_BlocksCache.Clean();
	m_BlocksCache.SetLimits(nBlolocksMemKB, nBlocksTimeout);
	m_AccountsCache.SetLimits(nAccMemKB, nAccTimeout);

	// 5. Set pre-encrypted values for CAccount
	CAccount::SetPsw(m_strKey);
	MsgBlockInitText();
	return m_Status;
}

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
CStorage::Status CStorage::GetStatus()
{
	return m_Status;
}

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
const char* CStorage::GetStatusStr()
{
	return GetStatusStr(m_Status);
}

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
const char* CStorage::GetStatusStr(const Status& Status)
{
	switch(Status)
	{
		case ReadyToOpen: return "ReadyToOpen";
		case Open 		: return "Open";
		case Error 		: return "Error";
		case NotInit	: return "Not initialized";
		case Unknown	: return "Unknown";
	}
	return "Invalid value.";
}

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
void CStorage::InitBytesArray()
{
	CCoderBF CoderBF;
	CoderBF.Init(m_strKey);

	CZorgeString strOne, strEncr;
	const char* szData = 0;
	for(int n = 0; n <= TOTAL_BYTES; ++n)
	{
		strOne.Format("%02X", n);
		CoderBF.EncryptToHexChars(strOne, strEncr);

		szData = (const char*)strEncr;
		size_t nLen = strlen(szData);
		char* sz = new char[nLen + 1];
		strcpy(sz, szData);
		m_arLetter[n] = sz;
	}
}

static void CheckLimits(const CZorgeString& str, unsigned int nMin, unsigned int nMax, const char* szName)
{
	unsigned int n = str.GetLength();
	if(n < nMin)
	{
		CZorgeString strErr;
		strErr.Format("%s is too short.", szName);
		throw CZorgeError(strErr);
	}
	if(n > nMax)
	{
		CZorgeString strErr;
		strErr.Format("%s is too long.", szName);
		throw CZorgeError(strErr);
	}
}

// private
void CStorage::GenerateConfigFile()
{
	CZorgeString strConfValue, strTmp;
	GuidStr(strConfValue);
	m_Config.Clear();
	m_Config.SetValue(CONFIG_REPO_ID, strConfValue);
	m_Config.SetComment(CONFIG_REPO_ID, "Unique repository id. Do not change it.");

	if(m_Coder.EncryptToHexChars(strConfValue, strTmp) != CODER_OK)
		throw CZorgeAssert("Failed to create repo key.");
	m_Config.SetValue(CONFIG_REPO_KEY, strTmp);
	m_Config.SetComment(CONFIG_REPO_KEY, "Repository key. Do not change it.");

	strConfValue = "7530";
	m_Config.SetValue(CONFIG_LISTENER_PORT, strConfValue);
	m_Config.SetComment(CONFIG_LISTENER_PORT, "Listener port.");

	strConfValue = "10";
	m_Config.SetValue(CONFIG_THREAD_POOL_SIZE, strConfValue);
	m_Config.SetComment(CONFIG_THREAD_POOL_SIZE, "Thread pool size. Value from 1");

	strConfValue = "5";
	m_Config.SetValue(CONFIG_THREAD_POOL_MONITOR_TIMEOUT, strConfValue);
	m_Config.SetComment(CONFIG_THREAD_POOL_MONITOR_TIMEOUT, "Thread pool monitor timeout. Value in seconds from 1");

	strConfValue = "128";
	m_Config.SetValue(CONFIG_CODER_POOL_MAX, strConfValue);
	m_Config.SetComment(CONFIG_CODER_POOL_MAX, "Encoders pool size.");

	strConfValue = "1024";
	m_Config.SetValue(CONFIG_BLOCKS_CACHE_MEM_KB, strConfValue);
	m_Config.SetComment(CONFIG_BLOCKS_CACHE_MEM_KB, "Max memory for blocks cache.");
	m_Config.SetComment(CONFIG_BLOCKS_CACHE_MEM_KB, "Expected value in KB.");

	strConfValue = "5";
	m_Config.SetValue(CONFIG_BLOCKS_CACHE_TIMEOUT, strConfValue);
	m_Config.SetComment(CONFIG_BLOCKS_CACHE_TIMEOUT, "Timeout for blocks cache before reporting 'core is too busy'.");
	m_Config.SetComment(CONFIG_BLOCKS_CACHE_TIMEOUT, "Expected value in seconds.");

	strConfValue = "1024";
	m_Config.SetValue(CONFIG_ACC_CACHE_MEM_KB, strConfValue);
	m_Config.SetComment(CONFIG_ACC_CACHE_MEM_KB, "Max memory for accounts cache.");
	m_Config.SetComment(CONFIG_ACC_CACHE_MEM_KB, "Expected value in KB.");

	strConfValue = "5";
	m_Config.SetValue(CONFIG_ACC_CACHE_TIMEOUT, strConfValue);
	m_Config.SetComment(CONFIG_ACC_CACHE_TIMEOUT, "Timeout for accounts cache before reporting 'core is too busy'.");
	m_Config.SetComment(CONFIG_ACC_CACHE_TIMEOUT, "Expected value in seconds.");

	strConfValue = "0";
	m_Config.SetValue(CONFIG_REPLY_TRACE, strConfValue);
	m_Config.SetComment(CONFIG_REPLY_TRACE, "Enable trace for reply and reply cache.");
	m_Config.SetComment(CONFIG_REPLY_TRACE, "1-enable, 0-disable.");

	strConfValue = "60";
	m_Config.SetValue(CONFIG_REPLY_CHACHE_TTL, strConfValue);
	m_Config.SetComment(CONFIG_REPLY_CHACHE_TTL, "Reply cache 'time to live'.");
	m_Config.SetComment(CONFIG_REPLY_CHACHE_TTL, "Value in seconds.");
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void CStorage::Init(const CZorgeString& strKey)
{
	if(strKey.IsEmpty())
		throw CZorgeError("Key is empty.");

	if(m_strRoot.IsEmpty())
		throw CZorgeError("Root is empty.");

	m_BlocksCache.Clean();
	m_AccountsCache.Clean();

	const char* szRoot = (const char*)m_strRoot;
	if(IsRepoEmpty(szRoot) == false && m_Status != Open)
		throw CZorgeError("Storage is not empty. Login required.");

	// Delete all files and folders.
	DeleteFilesTree((const char*)m_strRoot, 0);
	if(!IsFolderEmpty((const char*)m_strRoot))
		throw CZorgeError("Failed to clean storage.");

	m_Coder.Init(strKey);

	// 1. Generate config file.
	CZorgeString strConfigData, strConfigDataEncr;
	GenerateConfigFile();
	m_Config.ToString(strConfigData);

	// 2. Store config file
	CZorgeString strConfigPath(m_strRoot);
	strConfigPath += "config";

	if(!StringToFile(strConfigPath, strConfigData))
		throw CZorgeAssert("Failed to store config.");

	Login(strKey);
}

static void GetCleanNumber(const CZorgeString& strSource, CZorgeString& strCleanNum)
{
	strCleanNum.EmptyData();
	unsigned int nDigitsFound = 0;

	const char* szCur = (const char*)strSource;
	for(; *szCur != 0; ++szCur)
	{
		char ch = *szCur;
		if(ch >= '0' && ch <= '9')
		{
			strCleanNum += ch;
			if(++nDigitsFound > MAX_DIGITS_IN_NUMBER)
				throw CZorgeError("Number is too long.");
		}
	}
	if(nDigitsFound < MIN_DIGITS_IN_NUMBER)
		throw CZorgeError("Number is too short.");
}

// private
void CStorage::GetDigits(const CZorgeString& strSource, CZorgeString& strOut, TStrings& vOut, CZorgeString& strCleanNum)
{
	strCleanNum.EmptyData();
	vOut.clear();
	unsigned int nDigitsFound = 0;

	strOut.EmptyData();
	const char* szCur = (const char*)strSource;
	for(; *szCur != 0; ++szCur)
	{
		char ch = *szCur;
		if(ch >= '0' && ch <= '9')
		{
			strCleanNum += ch;
			const char* p = m_arLetter[(int)ch];
			if(p == 0)
				throw CZorgeAssert("Assert (p==0) at CStorage::GetDigits");
			strOut += p;
			strOut += "/";
			if(++nDigitsFound > MAX_DIGITS_IN_NUMBER)
				throw CZorgeError("Number is too long.");

			vOut.push_back(CZorgeString(p));
		}
	}
	if(nDigitsFound < MIN_DIGITS_IN_NUMBER)
		throw CZorgeError("Number is too short.");
}

// private
void CStorage::GetCleanNumWithPath(const CZorgeString& strSource, CZorgeString& strPath, CZorgeString& strCleanNum)
{
	strCleanNum.EmptyData();
	strPath.EmptyData();
	unsigned int nDigitsFound = 0;

	const char* szCur = (const char*)strSource;
	for(; *szCur != 0; ++szCur)
	{
		char ch = *szCur;
		if(ch >= '0' && ch <= '9')
		{
			strCleanNum += ch;
			const char* p = m_arLetter[(int)ch];
			if(p == 0)
				throw CZorgeAssert("Assert (p==0) at CStorage::GetDigits");
			strPath += p;
			strPath += "/";
			if(++nDigitsFound > MAX_DIGITS_IN_NUMBER)
				throw CZorgeError("Number is too long.");
		}
	}
	if(nDigitsFound < MIN_DIGITS_IN_NUMBER)
		throw CZorgeError("Number is too short.");
}

// private
void CStorage::GetCleanNum(const CZorgeString& strSource, CZorgeString& strCleanNum)
{
	strCleanNum.EmptyData();
	unsigned int nDigitsFound = 0;

	const char* szCur = (const char*)strSource;
	for(; *szCur != 0; ++szCur)
	{
		char ch = *szCur;
		if(ch >= '0' && ch <= '9')
		{
			strCleanNum += ch;
			const char* p = m_arLetter[(int)ch];
			if(p == 0)
				throw CZorgeAssert("Assert (p==0) at CStorage::GetDigits");

			if(++nDigitsFound > MAX_DIGITS_IN_NUMBER)
				throw CZorgeError("Number is too long.");
		}
	}
	if(nDigitsFound < MIN_DIGITS_IN_NUMBER)
		throw CZorgeError("Number is too short.");
}

static bool CreateNumberRoot(const CZorgeString& strRoot, const TStrings& vStrings)
{
	if(!vStrings.size())
		return false; // new number is empty

	int nRc = 0;
	CZorgeString strCur(strRoot);

	TStrings::const_iterator it = vStrings.begin();
	for(; it != vStrings.end(); ++it)
	{
		strCur += *it;
		strCur += "/";
		nRc = MkDir((const char*)strCur);
		if(nRc != 0)
		{
			printf("Failed to create '%s', errno = %d\n", (const char*)strCur, nRc);
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void CStorage::AddNumber(const CZorgeString& strNewNum, const CZorgeString& strPin, CZorgeString& strCreatedNumber)
{
	if(m_Status != Open)
		throw CZorgeError("Can not add number, repository is not open. Do login first.");

	strCreatedNumber.EmptyData();
	CheckLimits(strPin, MIN_DIGITS_IN_PIN, MAX_DIGITS_IN_PIN, "Pin");

	// 1-2-3 -> 31/32/33/
	CZorgeString strNewNumPath;
	TStrings vNumber;
	GetDigits(strNewNum, strNewNumPath, vNumber, strCreatedNumber);

	CheckLimits(strCreatedNumber, MIN_DIGITS_IN_NUMBER, MAX_DIGITS_IN_NUMBER, "Number");

	CLockerGuard G(m_Locker, strCreatedNumber);
	CZorgeString strAccRoot(m_strRoot);
	strAccRoot += strNewNumPath;

	if(CAccount::IsAccountFileExist(strAccRoot))
		throw CZorgeError("Number already exist.");

	CAccountCacheItem* pItem = m_AccountsCache.Find(&strCreatedNumber);
	if(pItem)
	{
		pItem->Release();
		throw CZorgeError("Number already exist.");
	}
	// if account not found in the cache, it does not means that it does not exist.

	if(!CreateNumberRoot(m_strRoot, vNumber))
		throw CZorgeAssert("Failed to create number in storage.");

	CAccount* pAcc = new CAccount(strCreatedNumber, strAccRoot, strPin);
	pItem = m_AccountsCache.AddAndLock(pAcc);
	pItem->Release();

	pAcc->CreateInFolder(strPin);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
bool CStorage::FindNumber(const CZorgeString& strNum)
{
	if(m_Status != Open)
		throw CZorgeError("Can not find number, repository is not open. Do login first.");

	CZorgeString strCleanNum;
	GetCleanNumber(strNum, strCleanNum);

	CLockerGuard G(m_Locker, strCleanNum);
	CAccountCacheItem* pItem = FindAccount(strCleanNum);
	if(pItem == 0)
		return false;
	pItem->Release();
	return true;
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void CStorage::DeleteNumber(const CZorgeString& strNum, const CZorgeString& strPin)
{
	if(m_Status != Open)
		throw CZorgeError("Can not delete number, repository is not open. Do login first.");

	CheckLimits(strPin, MIN_DIGITS_IN_PIN, MAX_DIGITS_IN_PIN, "Pin");

	CZorgeString strCleanNum;
	GetCleanNum(strNum, strCleanNum);

	CLockerGuard G(m_Locker, strCleanNum);
	CAccountCacheItem* pItem = FindAccount(strCleanNum);
	if(pItem == 0)
		throw CZorgeError("Number does not exist.");

	CAccountCacheItemGuard Guard(pItem);

	CAccount* pAcc = pItem->GetAccount();
	TLines vCacheKeys;
	pAcc->Delete(strPin, vCacheKeys);
	if(vCacheKeys.size())
	{
		TLines::const_iterator it = vCacheKeys.begin();
		for(; it != vCacheKeys.end(); ++it)
			m_BlocksCache.Delete(*it);
	}

	m_AccountsCache.Delete(&strCleanNum); // release memory for pAcc
	// pItem at this point released. Guard will access released object in the destructor.
	// Order of destructor calls is important.
	Guard.Set(0);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void CStorage::OpenNumber(const CZorgeString& strNum, const CZorgeString& strPin)
{
	if(m_Status != Open)
		throw CZorgeError("Can not open number, repository is not open. Do login first.");

	CZorgeString strCleanNumFrom;
	GetCleanNum(strNum, strCleanNumFrom);

	CheckLimits(strCleanNumFrom, MIN_DIGITS_IN_NUMBER, MAX_DIGITS_IN_NUMBER, "Number");
	CheckLimits(strPin, MIN_DIGITS_IN_PIN, MAX_DIGITS_IN_PIN, "Pin");

	CLockerGuard G(m_Locker, strCleanNumFrom);
	CAccountCacheItem* pItem = FindAccount(strCleanNumFrom);
	if(!pItem)
		throw CZorgeError("Number does not exist.");

	CAccountCacheItemGuard Guard(pItem);
	CAccount* pAcc = pItem->GetAccount();

	pAcc->Open(strPin);
}

// private
CAccountCacheItem* CStorage::FindAccount(const CZorgeString& strNum)
{
	CAccountCacheItem* pItem = m_AccountsCache.Find(&strNum);
	if(pItem)
		return pItem;

	//printf(".....Loading account %s\n", (const char*)strNum);
	CZorgeString strPath, strCleanNumber;
	GetCleanNumWithPath(strNum, strPath, strCleanNumber);

	CZorgeString strRoot(m_strRoot);
	strRoot += strPath;

	strPath = strRoot;
	strPath += g_strConfigEncr;

	// Check if config file exist for account
	if(IsFileExist((const char*)strPath) != FILE_EXIST)
		return 0; // account root folder or config file does not exist.

	CAccount* pAcc = new CAccount(strCleanNumber, strRoot);
	pItem = m_AccountsCache.AddAndLock(pAcc);

	return pItem;
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void CStorage::GetMsgCount(const CNumDev& From, unsigned int& nTotal, unsigned int& nNew)
{
	if(m_Status != Open)
		throw CZorgeError("Can not check messages, repository is not open. Do login first.");

	CZorgeString strCleanNumFrom;
	GetCleanNum(From.m_strNum, strCleanNumFrom);

	CLockerGuard G(m_Locker, strCleanNumFrom);
	CAccountCacheItem* pItem = FindAccount(strCleanNumFrom);
	if(!pItem)
		throw CZorgeError("Number does not exist.");

	CAccountCacheItemGuard Guard(pItem);
	CAccount* pAcc = pItem->GetAccount();

	pAcc->GetMsgCount(m_BlocksCache, From.m_strPin, nTotal, nNew);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void CStorage::SendMsg(const CNumDev& From, const CZorgeString& strToNum, const CZorgeString& strMsg, CZorgeString& strMsgGuid)
{
	if(m_Status != Open)
		throw CZorgeError("Can not send message, repository is not open. Do login first.");

	// Check for 'From' can be done once in the Open()
	CheckLimits(strToNum, MIN_DIGITS_IN_NUMBER, MAX_DIGITS_IN_NUMBER, "Number");
	CheckLimits(From.m_strNum, MIN_DIGITS_IN_NUMBER, MAX_DIGITS_IN_NUMBER, "Number");
	CheckLimits(From.m_strPin, MIN_DIGITS_IN_PIN, MAX_DIGITS_IN_PIN, "Pin");
	if(strMsg.GetLength() > MAX_MSG_LEN_BYTES)
	{
		CZorgeString strErr("Message is too long");
		throw CZorgeError(strErr);
	}

	CZorgeString strCleanNumFrom, strCleanNumTo;
	GetCleanNum(From.m_strNum, strCleanNumFrom);

	// Scope to temp lock 'from' to check its pin.
	// Only one account locked, to prevent dead lock.
	{
		CLockerGuard GFrom(m_Locker, strCleanNumFrom);
		CAccountCacheItem* pItemFrom = FindAccount(strCleanNumFrom);
		if(!pItemFrom)
			throw CZorgeError("Number 'from' does not exist.");

		CAccountCacheItemGuard GuardFrom(pItemFrom);
		CAccount* pAccFrom = pItemFrom->GetAccount();
		pAccFrom->CheckPin(From.m_strPin, true);
	}

	GetCleanNum(strToNum, strCleanNumTo);
	CLockerGuard G(m_Locker, strCleanNumTo);
	CAccountCacheItem* pItemTo = FindAccount(strCleanNumTo);
	if(!pItemTo)
		throw CZorgeError("Number 'to' does not exist.");

	CAccountCacheItemGuard GuardTo(pItemTo);
	CAccount* pAccTo = pItemTo->GetAccount();
	pAccTo->SendMsg(m_BlocksCache, From.m_strPin, strCleanNumFrom, strMsg, strMsgGuid);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void CStorage::GetIds(const CNumDev& From, unsigned int nType, TIds& vIds)
{
	if(m_Status != Open)
		throw CZorgeError("Can not get message ids, repository is not open. Do login first.");

	CZorgeString strCleanNum;
	GetCleanNum(From.m_strNum, strCleanNum);

	CLockerGuard G(m_Locker, strCleanNum);
	CAccountCacheItem* pItem = FindAccount(strCleanNum);
	if(!pItem)
		throw CZorgeError("Number 'to' does not exist.");

	CAccountCacheItemGuard Guard(pItem);
	CAccount* pAcc = pItem->GetAccount();

	pAcc->GetIds(m_BlocksCache, From.m_strPin, nType, vIds);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
CMessage* CStorage::ReadMessage(const CNumDev& From, const CZorgeString& strMsgId)
{
	if(m_Status != Open)
		throw CZorgeError("Can not read messages, repository is not open. Do login first.");

	CheckLimits(From.m_strPin, MIN_DIGITS_IN_PIN, MAX_DIGITS_IN_PIN, "Pin");
	CZorgeString strCleanNum;
	GetCleanNum(From.m_strNum, strCleanNum);

	CheckLimits(From.m_strNum, MIN_DIGITS_IN_NUMBER, MAX_DIGITS_IN_NUMBER, "Number");

	CLockerGuard G(m_Locker, strCleanNum);
	CAccountCacheItem* pItem = FindAccount(strCleanNum);
	if(!pItem)
		throw CZorgeError("Number 'from' does not exist.");

	CAccountCacheItemGuard Guard(pItem);
	CAccount* pAcc = pItem->GetAccount();

	return pAcc->ReadMessage(m_BlocksCache, From.m_strPin, strMsgId);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
int CStorage::DeleteMessage(const CNumDev& From, const CZorgeString& strMsgId)
{
	if(m_Status != Open)
		throw CZorgeError("Can not delete messages, repository is not open. Do login first.");

	CheckLimits(From.m_strPin, MIN_DIGITS_IN_PIN, MAX_DIGITS_IN_PIN, "Pin");
	CZorgeString strCleanNum;
	GetCleanNum(From.m_strNum, strCleanNum);

	CheckLimits(From.m_strNum, MIN_DIGITS_IN_NUMBER, MAX_DIGITS_IN_NUMBER, "Number");

	if(strMsgId.IsEmpty())
		throw CZorgeError("Message id is empty.");

	CLockerGuard G(m_Locker, strCleanNum);
	CAccountCacheItem* pItem = FindAccount(strCleanNum);
	if(!pItem)
		throw CZorgeError("Number 'from' does not exist.");

	CAccountCacheItemGuard Guard(pItem);
	CAccount* pAcc = pItem->GetAccount();

	return pAcc->DeleteMessage(m_BlocksCache, From.m_strPin, strMsgId);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void CStorage::Diagnostics(CZorgeString& strOut)
{
	// FIX ME it should show % of used.
	m_AccountsCache.Diagnostics(strOut);
	m_BlocksCache.Diagnostics(strOut);

	DiagCZorgeString(strOut);
	DiagCAccountCacheItem(strOut);
	DiagCBlocksCacheItem(strOut);
	DiagCAccount(strOut);
	DiagCCoderAES(strOut);
	DiagCCoderPoolItem(strOut);
	DiagCCoderBF(strOut);
	DiagCCoderRSA(strOut);
	DiagCConfig(strOut);
	DiagCMemBuffer(strOut);
	DiagCMsgBlock(strOut);
}

//---------------------------------------------------------------------
// Change pin.
//---------------------------------------------------------------------
void CStorage::ChangePin(const CNumDev& Acc, const CZorgeString& strNewPin)
{
	if(m_Status != Open)
		throw CZorgeError("Can not change pin, repository is not open. Do login first.");

	CheckLimits(Acc.m_strPin, MIN_DIGITS_IN_PIN, MAX_DIGITS_IN_PIN, "Pin");
	CheckLimits(strNewPin, MIN_DIGITS_IN_PIN, MAX_DIGITS_IN_PIN, "Pin");

	CZorgeString strCleanNum;
	GetCleanNum(Acc.m_strNum, strCleanNum);

	CLockerGuard G(m_Locker, strCleanNum);
	CAccountCacheItem* pItem = FindAccount(strCleanNum);
	if(!pItem)
		throw CZorgeError("Number does not exist.");

	CAccountCacheItemGuard Guard(pItem);
	CAccount* pAcc = pItem->GetAccount();

	return pAcc->ChangePin(Acc.m_strPin, strNewPin);
}

//---------------------------------------------------------------------
// Get account token.
//---------------------------------------------------------------------
const CZorgeString& CStorage::GetNumToken(const CNumDev& Num)
{
	if(m_Status != Open)
		throw CZorgeError("Repository is not open. Do login first.");

	CZorgeString strCleanNumFrom;
	GetCleanNum(Num.m_strNum, strCleanNumFrom);

	CLockerGuard G(m_Locker, strCleanNumFrom);
	CAccountCacheItem* pItem = FindAccount(strCleanNumFrom);
	if(!pItem)
		throw CZorgeError("Number does not exist.");

	CAccountCacheItemGuard Guard(pItem);
	CAccount* pAcc = pItem->GetAccount();

	return pAcc->GetToken();
}

//---------------------------------------------------------------------
// Generate new account token.
//---------------------------------------------------------------------
const CZorgeString& CStorage::NewNumToken(const CNumDev& Num)
{
	if(m_Status != Open)
		throw CZorgeError("Repository is not open. Do login first.");

	CZorgeString strCleanNumFrom;
	GetCleanNum(Num.m_strNum, strCleanNumFrom);

	CLockerGuard G(m_Locker, strCleanNumFrom);
	CAccountCacheItem* pItem = FindAccount(strCleanNumFrom);
	if(!pItem)
		throw CZorgeError("Number does not exist.");

	CAccountCacheItemGuard Guard(pItem);
	CAccount* pAcc = pItem->GetAccount();

	return pAcc->NewToken();
}

//---------------------------------------------------------------------
// Delete account token.
//---------------------------------------------------------------------
void CStorage::DeleteNumToken(const CNumDev& Num)
{
	if(m_Status != Open)
		throw CZorgeError("Repository is not open. Do login first.");

	CZorgeString strCleanNumFrom;
	GetCleanNum(Num.m_strNum, strCleanNumFrom);

	CLockerGuard G(m_Locker, strCleanNumFrom);
	CAccountCacheItem* pItem = FindAccount(strCleanNumFrom);
	if(!pItem)
		throw CZorgeError("Number does not exist.");

	CAccountCacheItemGuard Guard(pItem);
	CAccount* pAcc = pItem->GetAccount();

	pAcc->DeleteToken();
}

//---------------------------------------------------------------------
// Check account token.
//---------------------------------------------------------------------
bool CStorage::CheckNumToken(const CNumDev& Num)
{
	if(m_Status != Open)
		throw CZorgeError("Repository is not open. Do login first.");

	CZorgeString strCleanNumFrom;
	GetCleanNum(Num.m_strNum, strCleanNumFrom);

	CLockerGuard G(m_Locker, strCleanNumFrom);
	CAccountCacheItem* pItem = FindAccount(strCleanNumFrom);
	if(!pItem)
		throw CZorgeError("Number does not exist.");

	CAccountCacheItemGuard Guard(pItem);
	CAccount* pAcc = pItem->GetAccount();

	return pAcc->CheckToken(Num.m_strPin);
}

/*
Repository root
 +- Repo config file 		<- not encrypted
 +- F1 						<- RP
    +- Account config file 	<- RP
    	+- config token 	<- AP
    +- Message block file 	<- not encrypted (may be num with BF+RP)
    +- msg1 				<- RP
*/

