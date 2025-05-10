//MsgBlock.h

#ifndef CORE_MSGBLOCK_H_
#define CORE_MSGBLOCK_H_

#include <time.h>
#include <atomic>
#include "ZorgeString.h"
#include "Globals.h"

#define MSG_ALL 			0
#define MSG_NEW 			1

#define MSG_STATUS_ERROR	0
#define MSG_STATUS_NEW 		1
#define MSG_STATUS_READ 	2
#define MSG_STATUS_DELETED	3

#define MSG_STATUS_NEW_CHAR 'N'
#define MSG_STATUS_DEL_CHAR 'D'
#define MSG_STATUS_READ_CHAR 'R'

typedef vector<CZorgeString*> TIds;

#define MAX_MSG_FILE_LEN 	 32 // GUID in letters
#define MIN_MSG_FILE_LEN 	 1  //

#pragma pack(push, 1)
struct CMsgBlockHeader
{
	unsigned char 	m_chVersion;
	unsigned int 	m_nTotal;
	unsigned int 	m_nNew;
	unsigned int 	m_nDeleted;
};

struct CMsgBlockRecordData
{
	unsigned char	m_nStatus;
	time_t			m_nTimeCreated;
	char			m_szFromNum[MAX_DIGITS_IN_NUMBER + 1];
	char			m_szFileName[MAX_MSG_FILE_LEN + 1];
};
#pragma pack(pop)

struct CMsgBlockRecord
{
	CMsgBlockRecordData m_Data;

	CMsgBlockRecord();
	CMsgBlockRecord(const char* szFromNum, const char* szFileName);
	CMsgBlockRecord* Clone();
	//~CMsgBlockRecord();
};

struct CMessage
{
	CMessage(const CMessage&) = delete;
	CMessage& operator= (const CMessage&) = delete;

	CMessage();
	CMessage(CMsgBlockRecord* pRec);
	void Print();
	CMessage* Clone();

	CMsgBlockRecord	m_Record;
	CZorgeString	m_strText;
};

#define MSG_DELETED 		1
#define MSG_ALREADY_DELETED 2
//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
class CMsgBlock
{
	int				m_nHandler;

	CMsgBlockHeader	m_Header;
	bool			m_bHeaderLoaded;
	unsigned int	m_nMaxCount;	// Max count of messages in one block.

	size_t			m_nMemSize;
	char*			m_pMem;

	CZorgeString	m_strRoot;

	unsigned int 	GetOffsetForNextRecord();
	void			Open();
	void 			LoadHeader();
	void 			StoreHeader();
	bool 			UpdateStoreStatus(CMsgBlockRecord*, size_t, unsigned int);
	void			Compress();
public:
	CZorgeString	m_strFullPath; // make it public to print debug info.

	static std::atomic<unsigned int> g_nTotalOpenHandlers;
	static std::atomic<unsigned long> g_nTotalMemory;

	CMsgBlock(const CMsgBlock&) = delete;
	CMsgBlock operator=(CMsgBlock&) = delete;

	CMsgBlock(const CZorgeString&);
	~CMsgBlock();

	void			LoadAllRecords();
	void			PrintHeader();
	void 			Print();

	// Interface
	void 			GetCount(unsigned int& nTotal, unsigned int&nNew);
	void			Add(const CZorgeString&, const CZorgeString&);
	void			GetIds(unsigned int, TIds&);
	void			CloseHandler();
	void 			ReleaseMemory();
	bool			IsHandleOpen();
	size_t			GetMemSize();
	CMessage*		ReadMessage(const CZorgeString&);
	int 			DeleteMessage(const CZorgeString&);
};

struct CMessageDetails
{
	CZorgeString	m_strStatus;
	CZorgeString	m_strTimeCreated;
	CZorgeString	m_strFromNum;
	CZorgeString	m_strId;
};

bool ParseMessageLine(const CZorgeString& strLine, CMessageDetails& Out);
void MsgBlockInitText();
const char* MessageStatusToString(unsigned int nStatus);

#endif /* CORE_MSGBLOCK_H_ */
