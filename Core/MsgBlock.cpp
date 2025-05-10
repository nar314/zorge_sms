#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#include "MsgBlock.h"
#include "SysUtils.h"
#include "StrUtils.h"
#include "CoderAESPool.h"

#define CUR_HEADER_VER 1
#define MAX_PERCENT_BLOCK_COMPRESS 80

static const unsigned int g_nDefMaxMessages = MAX_MSG_IN_BLOCK;

static const size_t g_nHeaderLen = sizeof(CMsgBlockHeader);
static const size_t g_nRecordLen = sizeof(CMsgBlockRecord);

static CZorgeString g_strMsgInEncr = "msg.in";

std::atomic<unsigned int> CMsgBlock::g_nTotalOpenHandlers = 0;
std::atomic<unsigned long> CMsgBlock::g_nTotalMemory = 0;

std::atomic<unsigned int> g_nCMsgBlock = 0;
std::atomic<unsigned int> g_nCMsgBlock_max = 0;

void DiagCMsgBlock(CZorgeString& strOut)
{
	CZorgeString str;
	str.Format("CMsgBlock.\tCur=%d, Max=%d\n", (unsigned int)g_nCMsgBlock, (unsigned int)g_nCMsgBlock_max);
	strOut += str;
}

struct CMsgBlockCounter
{
	~CMsgBlockCounter()
	{
		printf("CMsgBlock = %d (%d)\n", (unsigned int)g_nCMsgBlock, (unsigned int)g_nCMsgBlock_max);
	}
	static void Add()
	{
		g_nCMsgBlock++;
		g_nCMsgBlock_max++;
	}
} g_CMsgBlockCounter;

CMsgBlockRecord::CMsgBlockRecord()
{
	memset(&m_Data, 0, sizeof(CMsgBlockRecordData));
	m_Data.m_nStatus = MSG_STATUS_ERROR;
}

CMsgBlockRecord::CMsgBlockRecord(const char* szFromNum, const char* szFileName)
{
	memset(&m_Data, 0, sizeof(CMsgBlockRecordData));
	m_Data.m_nStatus = MSG_STATUS_NEW;
	m_Data.m_nTimeCreated = GetUtcNow();
	strcpyz(m_Data.m_szFromNum, sizeof(m_Data.m_szFromNum), szFromNum);
	strcpyz(m_Data.m_szFileName, sizeof(m_Data.m_szFileName), szFileName);
}

CMsgBlockRecord* CMsgBlockRecord::Clone()
{
	CMsgBlockRecord* p = new CMsgBlockRecord();
	memcpy(&p->m_Data, &m_Data, sizeof(CMsgBlockRecordData));
	return p;
}

// Global
void MsgBlockInitText()
{
	// This method called from repo login
	CCoderPoolItem* pItem = g_RepoCoderPool.Get();
	CCoderPoolGuard Guard(pItem);
	CCoderAES* pCoder = pItem->m_pItem;

	if(pCoder->EncryptToHexChars("msg.in", g_strMsgInEncr) != CODER_OK)
		throw CZorgeAssert("Failed to encrypt account block file name.");
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
CMsgBlock::CMsgBlock(const CZorgeString& strRoot)
{
	CMsgBlockCounter::Add();

	m_nHandler = -1;
	m_pMem = 0;
	m_nMemSize = 0;

	m_nMaxCount = g_nDefMaxMessages;
	m_Header.m_chVersion = CUR_HEADER_VER;
	m_Header.m_nNew = 0;
	m_Header.m_nTotal = 0;
	m_Header.m_nDeleted = 0;
	m_bHeaderLoaded = false;

	m_strRoot = strRoot;
	if(m_strRoot.GetLastChar() != '/')
		m_strRoot += "/";
	m_strFullPath = m_strRoot;
	m_strFullPath += g_strMsgInEncr;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
CMsgBlock::~CMsgBlock()
{
	g_nCMsgBlock--;
	if(m_nHandler != -1)
	{
		close(m_nHandler);
		g_nTotalOpenHandlers--;
	}

	if(m_pMem)
	{
		if(m_nMemSize)
		{
			g_nTotalMemory -= m_nMemSize;
			//printf("1- %ld\n", m_nMemSize);
		}
		delete [] m_pMem;
	}
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void CMsgBlock::CloseHandler()
{
	if(m_nHandler == -1)
		return;

	close(m_nHandler);
	m_nHandler = -1;
	g_nTotalOpenHandlers--;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void CMsgBlock::ReleaseMemory()
{
	if(m_pMem == 0)
		return;

	if(m_nMemSize)
	{
		g_nTotalMemory -= m_nMemSize;
		//printf("1- %ld\n", m_nMemSize);
	}
	delete [] m_pMem;
	m_pMem = 0;
	m_nMemSize = 0;
}

static bool IsCompress(unsigned int nDeleted, unsigned int nTotal)
{
	// Block has to be more than half full
	if(nTotal >= g_nDefMaxMessages / 2)
	{
		// and percent of deleted messages should be > MAX_PERCENT_BLOCK_COMPRESS
		float pr = (float)nDeleted * 100.0/ (float)nTotal;
		return pr > MAX_PERCENT_BLOCK_COMPRESS;
	}
	return false;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
bool CMsgBlock::IsHandleOpen()
{
	return m_nHandler != -1;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void CMsgBlock::LoadHeader()
{
	if(m_nHandler == -1)
		Open();

	if(lseek(m_nHandler, 0, SEEK_SET) == -1)
		throw CZorgeAssert("Assert(true) CMsgBlock::LoadHeader");

	size_t nRead = read(m_nHandler, &m_Header, g_nHeaderLen);
	if(nRead == 0)
	{
		m_bHeaderLoaded = true;
		return; // file is empty
	}

	if(nRead != g_nHeaderLen)
		throw CZorgeAssert("Failed to load message block header.");

	if(m_Header.m_chVersion != CUR_HEADER_VER)
		throw CZorgeAssert("Invalid header version.");
	m_bHeaderLoaded = true;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void CMsgBlock::StoreHeader()
{
	if(m_nHandler == -1)
		Open();

	if(lseek(m_nHandler, 0, SEEK_SET) == -1)
	{
		int n = errno;
		long t = (long)pthread_self();
		CZorgeString strErr;
		strErr.Format("Assert(true) CMsgBlock::StoreHeader, errno = %d, %s, %lX, nHandler=%d", n, (const char*)m_strFullPath, t, m_nHandler);
		printf("%s\n", (const char*)strErr);
		//abort();
		throw CZorgeAssert(strErr);
	}

	if(write(m_nHandler, &m_Header, g_nHeaderLen) != g_nHeaderLen)
	{
		int n = errno;
		long t = (long)pthread_self();
		CZorgeString strErr;
		strErr.Format("Assert(true) CMsgBlock::StoreHeader, write(), errno = %d, %s, %lX, nHandler=%d", n, (const char*)m_strFullPath, t, m_nHandler);
		printf("%s\n", (const char*)strErr);
		//abort();
		throw CZorgeAssert(strErr);
	}
/*
	if(fsync(m_nHandler) != 0)
	{
		int n = errno;
		long t = (long)pthread_self();
		CZorgeString strErr;
		strErr.Format("Assert(true) CMsgBlock::StoreHeader, fsync(), errno = %d, %s, %lX, nHandler=%d", n, (const char*)m_strFullPath, t, m_nHandler);
		printf("%s\n", (const char*)strErr);
		//abort();
		throw CZorgeAssert(strErr);
	}
*/
}

void CMsgBlock::PrintHeader()
{
	printf("[%d] new=%d total=%d deleted=%d\n", m_Header.m_chVersion, m_Header.m_nNew, m_Header.m_nTotal, m_Header.m_nDeleted);
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void CMsgBlock::Open()
{
	if(m_nHandler != -1)
		return; // That block already open

	// Open or create file.
	m_nHandler = open((const char*)m_strFullPath, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
	if(m_nHandler == -1)
	{
		int nErrno = errno;
		printf("Failed to open file '%s', errno = %d\n", (const char*)m_strFullPath, nErrno);
		return;
	}
	g_nTotalOpenHandlers++;
}

#define RECORDS_STEP 10
// private, called from Add()
unsigned int CMsgBlock::GetOffsetForNextRecord()
{
	size_t nOffsetEndNewRecord = (m_Header.m_nTotal + 1) * g_nRecordLen;
	if(nOffsetEndNewRecord > m_nMemSize)
	{
		int nNewMemSize = m_nMemSize + (g_nRecordLen * RECORDS_STEP);
		char* pNewMem = new char[nNewMemSize];
		if(m_nMemSize)
			memcpy(pNewMem, m_pMem, m_nMemSize);

		if(m_nMemSize)
		{
			//printf("2- %ld\n", m_nMemSize);
			g_nTotalMemory -= m_nMemSize;
		}
		delete [] m_pMem;
		m_pMem = pNewMem;
		m_nMemSize = nNewMemSize;
		g_nTotalMemory += m_nMemSize;
	}
	return m_Header.m_nTotal * g_nRecordLen;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void CMsgBlock::Add(const CZorgeString& strFromNum, const CZorgeString& strFileName)
{
	unsigned int n = strFromNum.GetLength();
	if(n > MAX_DIGITS_IN_NUMBER)
		throw CZorgeError("Number is too long");
	if(n < MIN_DIGITS_IN_NUMBER)
		throw CZorgeError("Number is too short");

	n = strFileName.GetLength();
	if(n > MAX_MSG_FILE_LEN)
		throw CZorgeError("File name is too long");
	if(n < MIN_MSG_FILE_LEN)
		throw CZorgeError("File name is too short");

	if(m_nHandler == -1)
		Open();
	if(!m_bHeaderLoaded)
		LoadHeader();

	if(m_Header.m_nTotal >= m_nMaxCount)
	{
		if(m_Header.m_nDeleted != 0)
		{
			Compress();
			if(m_Header.m_nTotal >= m_nMaxCount)
				throw CZorgeError("No space for new message. (1)");
		}
		else
			throw CZorgeError("No space for new message.");
	}

	CMsgBlockRecord Record((const char*)strFromNum, (const char*)strFileName);
	unsigned int nOffset = GetOffsetForNextRecord(); // do it before m_Header.m_nTotal++
	memcpy(m_pMem + nOffset, &Record, g_nRecordLen);

	++m_Header.m_nNew;
	++m_Header.m_nTotal;
	StoreHeader();

	if(lseek(m_nHandler, 0, SEEK_END) == -1)
		throw CZorgeAssert("Assert(true) CMsgBlock::Add. 1");

	if(write(m_nHandler, &Record, g_nRecordLen) != g_nRecordLen)
		throw CZorgeAssert("Assert(true) CMsgBlock::Add. 2");

	//if(fsync(m_nHandler) != 0)
		//throw CZorgeAssert("Assert(true) CMsgBlock::Add. 3");
}

static void PrintRecord(const CMsgBlockRecord* p)
{
	if(p)
		printf("%d, %ld [%s] [%s]\n", p->m_Data.m_nStatus, p->m_Data.m_nTimeCreated, p->m_Data.m_szFromNum, p->m_Data.m_szFileName);
	else
		puts("Assert p == 0");
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void CMsgBlock::LoadAllRecords()
{
	if(m_nHandler == -1)
		Open();
	if(!m_bHeaderLoaded)
		LoadHeader();

	if(m_Header.m_nTotal == 0)
	{
		if(m_pMem)
		{
			if(m_nMemSize)
			{
				g_nTotalMemory -= m_nMemSize;
				//printf("3- %ld\n", m_nMemSize);
			}
			delete [] m_pMem;
			m_pMem = 0;
		}
		m_nMemSize = 0;
		return;
	}

	unsigned int nAllocatedCount = m_nMemSize / g_nRecordLen;
	if(nAllocatedCount < m_Header.m_nTotal)
	{
		//puts("Need to re-allocate.");
		m_nMemSize = m_Header.m_nTotal * g_nRecordLen;
		m_pMem = new char[m_nMemSize];
		g_nTotalMemory += m_nMemSize;
	}

	if(lseek(m_nHandler, g_nHeaderLen, SEEK_SET) == -1)
		throw CZorgeAssert("Assert(true) CMsgBlock::LoadAllRecords2. 1");

	unsigned int nTotalRead = 0;
	while(1)
	{
		int nRead = read(m_nHandler, m_pMem + nTotalRead, m_nMemSize);
		if(nRead == 0)
		{
			puts("EOF");
			break; // eof
		}
		if(nRead == -1)
			throw CZorgeAssert("Assert(true) CMsgBlock::LoadAllRecords2. 2");
		nTotalRead += nRead;
		if(nTotalRead == m_nMemSize)
			break; // read all records
	}
}

void CMsgBlock::Print()
{
	if(m_pMem == 0 || m_nMemSize == 0)
		return;

	printf("Block root=%s\n", (const char*)m_strRoot);
	PrintHeader();
	unsigned int nLoadedCount = m_Header.m_nTotal;
	CMsgBlockRecord* pRec = (CMsgBlockRecord*)m_pMem;
	for(unsigned int i = 0; i < nLoadedCount; ++i)
	{
		printf("i = %d ", i);
		PrintRecord(pRec);
		++pRec;
	}
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
size_t CMsgBlock::GetMemSize()
{
	return m_nMemSize;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void CMsgBlock::GetCount(unsigned int& nTotal, unsigned int&nNew)
{
	if(m_nHandler == -1)
		Open();

	if(!m_bHeaderLoaded)
		LoadHeader();

	nTotal = m_Header.m_nTotal;
	nNew = m_Header.m_nNew;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void CMsgBlock::GetIds(unsigned int nType, TIds& vIds)
{
	if(vIds.size())
	{
		for(auto p : vIds)
			delete p;
		vIds.clear();
	}

	if(m_pMem == 0 || m_nMemSize == 0)
		LoadAllRecords();

	if(m_Header.m_nTotal == 0)
		return;

	if(nType == MSG_NEW && m_Header.m_nNew == 0)
		return;

	bool bAll = nType == MSG_ALL;
	bool bNew = nType == MSG_NEW;
	if(!bAll && !bNew)
		return; // What is that request ?

	unsigned int nLoadedCount = m_Header.m_nTotal;
	unsigned char chStatus = 0, chStatusChar = 0;
	char szTimeT[16];
	unsigned int nPreAllocate = 64 + MAX_DIGITS_IN_NUMBER;
	CMsgBlockRecord* pRec = (CMsgBlockRecord*)m_pMem;
	pRec = (CMsgBlockRecord*)m_pMem;

	for(unsigned int i = 0; i < nLoadedCount; ++i, ++pRec)
	{
		chStatus = pRec->m_Data.m_nStatus;

		if(bNew && chStatus != MSG_STATUS_NEW)
			continue;

		if(chStatus == MSG_STATUS_NEW)
			chStatusChar = MSG_STATUS_NEW_CHAR;
		else if(chStatus == MSG_STATUS_DELETED)
		{
			chStatusChar = MSG_STATUS_DEL_CHAR;
			continue; // do not include deleted messages into output
		}
		else if(chStatus == MSG_STATUS_READ)
			chStatusChar = MSG_STATUS_READ_CHAR;
		else
		{
			printf("Warning. Message has invalid status [%d]. seq=%d, %s\n", chStatus, i, (const char*)m_strFullPath);
			PrintHeader();
			continue; // Should not be happening.
		}

		snprintf(szTimeT, 16, "%ld", pRec->m_Data.m_nTimeCreated);
		CZorgeString* pLine = new CZorgeString(nPreAllocate);
		*pLine += chStatusChar;
		*pLine += " ";
		*pLine += szTimeT;
		*pLine += " ";
		*pLine += pRec->m_Data.m_szFromNum;
		*pLine += " ";
		*pLine += pRec->m_Data.m_szFileName;
		vIds.push_back(pLine);
	}
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
CMessage* CMsgBlock::ReadMessage(const CZorgeString& strId)
{
	// No need to check m_nHandler here, StoreHeader() or UpdateStoreStatus() will do if needed.
	if(m_pMem == 0 || m_nMemSize == 0)
		LoadAllRecords();

	if(m_Header.m_nTotal == 0)
		throw CZorgeError("No messages.");

	CZorgeString strTmp;
	unsigned int nLoadedCount = m_Header.m_nTotal;
	CMsgBlockRecord* pRec = (CMsgBlockRecord*)m_pMem;
	for(unsigned int i = 0; i < nLoadedCount; ++i, ++pRec)
	{
		if(strId == pRec->m_Data.m_szFileName)
		{
			CMsgBlockRecordData& Data = pRec->m_Data;
			if(Data.m_nStatus == MSG_STATUS_DELETED)
				throw CZorgeError("Message deleted.");

			strTmp = m_strRoot;
			strTmp += Data.m_szFileName;

			CCoderPoolItem* pItem = g_RepoCoderPool.Get();
			CCoderPoolGuard Guard(pItem);
			CCoderAES* pCoderRepo = pItem->m_pItem;

			CMemBuffer BufEncr, BufDecr;
			if(!FileToMem(strTmp, BufEncr))
				throw CZorgeAssert("Failed to load message.");

			if(pCoderRepo->Decrypt(BufEncr, BufDecr) != CODER_OK)
				throw CZorgeAssert("Failed to decrypt message.");

			CMessage* p = new CMessage(pRec);
			BufDecr.ToString(p->m_strText);

			// Update header and message status.
			if(Data.m_nStatus == MSG_STATUS_NEW)
			{
				m_Header.m_nNew--;
				StoreHeader();

				size_t nPos = (i + 1) * sizeof(CMsgBlockRecord) + sizeof(CMsgBlockHeader);
				UpdateStoreStatus(pRec, nPos, MSG_STATUS_READ);
			}
			return p;
		}
	}

	throw CZorgeError("Message not found.");
}

// private
bool CMsgBlock::UpdateStoreStatus(CMsgBlockRecord* pRec, size_t nFilePos, unsigned int nNewStatus)
{
	if(m_nHandler == -1)
		Open();

	pRec->m_Data.m_nStatus = nNewStatus;
	if(lseek(m_nHandler, nFilePos, SEEK_SET) != -1)
	{
		if(write(m_nHandler, &(pRec->m_Data.m_nStatus), 1) != 1)
		{
			puts("Assert(true) CMsgBlock::UpdateStoreStatus write failed. Message block not updated.");
			return false;
		}
		/*if(fsync(m_nHandler) != 0)
		{
			puts("Assert(true) CMsgBlock::UpdateStoreStatus fsync failed. Message block not updated.");
			return false;
		}*/
	}
	else
	{
		puts("Assert(true) CMsgBlock::UpdateStoreStatus lseek failed. Message block not updated.");
		return false;
	}
	return true;
}

//------------------------------------------------------------------------
// return true if deleted.
// return false if has been deleted before.
//------------------------------------------------------------------------
int CMsgBlock::DeleteMessage(const CZorgeString& strId)
{
	if(m_nHandler == -1)
		Open();

	if(m_pMem == 0 || m_nMemSize == 0)
		LoadAllRecords();

	if(m_Header.m_nTotal == 0)
		throw CZorgeError("Message not found.");

	CZorgeString strTmp;
	unsigned int nLoadedCount = m_Header.m_nTotal;
	CMsgBlockRecord* pRec = (CMsgBlockRecord*)m_pMem;
	for(unsigned int i = 0; i < nLoadedCount; ++i, ++pRec)
	{
		if(strId == pRec->m_Data.m_szFileName)
		{
			CMsgBlockRecordData& Data = pRec->m_Data;
			if(Data.m_nStatus == MSG_STATUS_DELETED)
				return MSG_ALREADY_DELETED; // nothing to do.

			strTmp = m_strRoot;
			strTmp += Data.m_szFileName;
			// Purge file m_strTmp
			int n = PurgeFile(strTmp);
			if (n != 0)
				printf("CMsgBlock::DeleteMessage Purge file return %d. %s\n", n, (const char*)strTmp);

			// Update header and message status.
			if(Data.m_nStatus == MSG_STATUS_NEW)
				m_Header.m_nNew--;

			m_Header.m_nDeleted++;
			StoreHeader();

			size_t nPos = (i * sizeof(CMsgBlockRecord)) + sizeof(CMsgBlockHeader);
			UpdateStoreStatus(pRec, nPos, MSG_STATUS_DELETED);

			if(m_Header.m_nDeleted > 0 && IsCompress(m_Header.m_nDeleted, m_Header.m_nTotal))
				Compress();

			return MSG_DELETED;
		}
	}
	throw CZorgeError("Message not found.");
	return 9; // make compiler happy
}

//------------------------------------------------------------------------
// private.
//------------------------------------------------------------------------
void CMsgBlock::Compress()
{
	if(m_nHandler == -1 || !m_bHeaderLoaded)
		return; // do nothing for not open block

	if(m_pMem == 0 || m_nMemSize == 0)
		return; // do nothing for not allocated block

	if(m_Header.m_nDeleted == 0)
		return; // block has no deleted records.

	//puts("Compressing.... Header before compression.");
	//PrintHeader();

	size_t nMemCopySize = 0;
	char* pMemCopy = new char[m_nMemSize];
	CMsgBlockRecord* pRecCopy = (CMsgBlockRecord*)pMemCopy;

	unsigned int nCompressedCount = 0;
	unsigned int nLoadedCount = m_Header.m_nTotal;
	CMsgBlockRecord* pRec = (CMsgBlockRecord*)m_pMem;
	for(unsigned int i = 0; i < nLoadedCount; i++, pRec++)
	{
		if(pRec->m_Data.m_nStatus == MSG_STATUS_DELETED)
		{
			++nCompressedCount;
			continue;
		}
		memcpy(pRecCopy, pRec, g_nRecordLen);
		++pRecCopy;
		nMemCopySize += g_nRecordLen;
	}

	if(nCompressedCount == 0 || nMemCopySize >= m_nMemSize || nLoadedCount < nCompressedCount)
	{
		// something wrong. Stop it
		printf("Assert(true) CMsgBlock::Compress. nMemCopySize=%ld, m_nMemSize=%ld, nLoadedCount=%d, nCompressedCount=%d\n", nMemCopySize, m_nMemSize, nLoadedCount, nCompressedCount);
		delete [] pMemCopy;
		return;
	}

	m_Header.m_nTotal -= nCompressedCount;
	m_Header.m_nDeleted = 0;
	memcpy(m_pMem, pMemCopy, nMemCopySize);

	// Store to file
	if(lseek(m_nHandler, 0, SEEK_SET) == -1)
		throw CZorgeAssert("Assert(true) CMsgBlock::Compress. 1");

	// write header
	if(write(m_nHandler, &m_Header, g_nHeaderLen) != g_nHeaderLen)
		throw CZorgeAssert("Assert(true) CMsgBlock::Compress. 2");

	// write block
	if(write(m_nHandler, m_pMem, nMemCopySize) != (int)nMemCopySize)
		throw CZorgeAssert("Assert(true) CMsgBlock::Compress. 3");

	//if(fsync(m_nHandler) != 0)
		//throw CZorgeAssert("Assert(true) CMsgBlock::Compress. 4");
}

CMessage::CMessage()
{
	// Should be called only from flow of Clone()
}

CMessage::CMessage(CMsgBlockRecord* pRec)
{
	memcpy(&m_Record, pRec, sizeof(CMsgBlockRecord));
}

void CMessage::Print()
{
	printf("%d, %ld [%s] [%s]\n", m_Record.m_Data.m_nStatus, m_Record.m_Data.m_nTimeCreated, m_Record.m_Data.m_szFromNum, m_Record.m_Data.m_szFileName);
	printf("%s\n", (const char*)m_strText);
}

CMessage* CMessage::Clone()
{
	CMessage* p = new CMessage();
	p->m_Record.Clone();
	p->m_strText = m_strText;
	return p;
}

// Global function
const char* MessageStatusToString(unsigned int nStatus)
{
	switch(nStatus)
	{
		case MSG_STATUS_ERROR 	: return "Error";
		case MSG_STATUS_NEW 	: return "New";
		case MSG_STATUS_READ 	: return "Read";
		case MSG_STATUS_DELETED : return "Deleted";
	}
	return "Invalid";
}

// Global function
bool ParseMessageLine(const CZorgeString& strLine, CMessageDetails& Out)
{
	char sz1[1024], sz2[1024], sz3[1024], sz4[1024];
	if(sscanf((const char*)strLine, "%s %s %s %s", sz1, sz2, sz3, sz4) != 4)
	{
		printf("Counter failed for ParseMessage, expected 4, in line [%s].\n", (const char*)strLine);
		return false;
	}

	Out.m_strStatus = sz1;
	Out.m_strTimeCreated = sz2;
	Out.m_strFromNum = sz3;
	Out.m_strId = sz4;
	return true;
}
