#include "Storage.h"
#include "SysUtils.h"
#include "StrUtils.h"
#include "ZorgeTimer.h"

CStorage* g_pStorageTest = 0;
unsigned int g_nTotalNumbers = 500;
unsigned int g_nFromNum = 100;
unsigned int g_nTotalMsg = 100;

static CZorgeTimer g_Timer;
static char g_szTimer[64];

extern void TestDeleteNumbers_Invalid();
extern void TestOpenNumbers_Invalid();
extern void TestMessageSend_Invalid();
extern void TestMessageRead_Invalid();
extern void TestMessageDelete_Invalid();
extern int Test_Thread(CStorage*, bool);
extern int TestChangePin_Invalid();

static void releaseIds(TIds& vIds)
{
	if(vIds.size())
	{
		for(auto p : vIds)
			delete p;
		vIds.clear();
	}
}

static void deleteAllNumbers()
{
	CZorgeString strNum, strPin, strNewNum;
	unsigned int nCurNum = g_nFromNum;
	for(unsigned int i = 0; i < g_nTotalNumbers; ++i, ++nCurNum)
	{
		strNum.Format("%d", nCurNum);
		strPin = strNum;

		if(g_pStorageTest->FindNumber(strNum))
			g_pStorageTest->DeleteNumber(strNum, strPin);
	}
}

void TestChangePin(const char* szNum)
{
	printf("--- TestChangePin ");
	g_Timer.Start();

	CZorgeString strTmp;
	CZorgeString strNum(szNum), strPin(szNum);

	if(g_pStorageTest->FindNumber(strNum))
		g_pStorageTest->DeleteNumber(strNum, strPin);
	g_pStorageTest->AddNumber(strNum, strPin, strTmp);

	CNumDev Num = { strNum, strPin };
	CZorgeString strNewPin = strPin;
	strNewPin += strPin;
	g_pStorageTest->ChangePin(Num, strNewPin);

	try
	{
		g_pStorageTest->OpenNumber(strNum, strPin);
		throw CZorgeAssert("Failed 1");
	}
	catch(CZorgeError& e)
	{
		if(e.GetMsg() != "Invalid pin")
			throw CZorgeAssert("Failed 2");
	}

	g_pStorageTest->OpenNumber(strNum, strNewPin);
	g_pStorageTest->DeleteNumber(strNum, strNewPin);
	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestDeleteNumbers()
{
	printf("--- TestDeleteNumbers ");
	g_Timer.Start();
	deleteAllNumbers();

	CZorgeString strNum, strPin, strNewNum;
	unsigned int nCurNum = g_nFromNum;
	for(unsigned int i = 0; i < g_nTotalNumbers; i++, nCurNum++)
	{
		strNum.Format("%d", nCurNum);
		strPin = strNum;
		g_pStorageTest->AddNumber(strNum, strPin, strNewNum);
	}

	nCurNum = g_nFromNum;
	for(unsigned int i = 0; i < g_nTotalNumbers; i++, nCurNum++)
	{
		strNum.Format("%d", nCurNum);
		strPin = strNum;
		g_pStorageTest->DeleteNumber(strNum, strPin);
	}

	// Checking numbers
	nCurNum = g_nFromNum;
	for(unsigned int i = 0; i < g_nTotalNumbers; i++, nCurNum++)
	{
		strNum.Format("%d", nCurNum);
		if(g_pStorageTest->FindNumber(strNum) == true)
		{
			CZorgeString strErr;
			strErr.Format("Number %s found, but should be deleted.", (const char*)strNum);
			throw CZorgeAssert(strErr);
		}
	}
	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestOpenNumbers()
{
	printf("--- TestOpenCloseNumbers ");
	g_Timer.Start();
	deleteAllNumbers();

	CZorgeString strNum, strPin, strNewNum;
	unsigned int nCurNum = g_nFromNum;
	for(unsigned int i = 0; i < g_nTotalNumbers; i++, nCurNum++)
	{
		strNum.Format("%d", nCurNum);
		strPin = strNum;
		g_pStorageTest->AddNumber(strNum, strPin, strNewNum);
	}
	// Open numbers
	nCurNum = g_nFromNum;
	for(unsigned int i = 0; i < g_nTotalNumbers; i++, nCurNum++)
	{
		strNum.Format("%d", nCurNum);
		strPin = strNum;
		g_pStorageTest->OpenNumber(strNum, strPin);
	}
	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void ParseMessage(const CZorgeString& strLine, CZorgeString& strStatus, CZorgeString& strTime, CZorgeString& strFrom, CZorgeString& strId)
{
	CMessageDetails Out;
	if(!ParseMessageLine(strLine, Out))
		throw CZorgeAssert("Counter failed.");

	strStatus = Out.m_strStatus;
	strTime = Out.m_strTimeCreated;
	strFrom = Out.m_strFromNum;
	strId = Out.m_strId;
}

#define MESSAGE_TO_SEND "This is default message sent form sender to receiver"
void TestMessageSend(const char* szFrom, const char* szTo)
{
	printf("--- TestMessageSend ");
	g_Timer.Start();

	CZorgeString strTmp;
	CZorgeString strNumSender(szFrom), strPinSender(szFrom);
	CZorgeString strNumReceiver(szTo), strPinReceiver(szTo);
	unsigned int i = 0;

	// Prepare number for sender
	if(g_pStorageTest->FindNumber(strNumSender))
		g_pStorageTest->DeleteNumber(strNumSender, strPinSender);
	g_pStorageTest->AddNumber(strNumSender, strPinSender, strTmp);

	// Prepare number for receiver
	if(g_pStorageTest->FindNumber(strNumReceiver))
		g_pStorageTest->DeleteNumber(strNumReceiver, strPinReceiver);
	g_pStorageTest->AddNumber(strNumReceiver, strPinReceiver, strTmp);

	// Send N messages
	vector<CZorgeString> vMsgIds;
	CZorgeString strMsgId;
	CNumDev Sender = { strNumSender, strPinSender};
	g_pStorageTest->OpenNumber(strNumSender, strPinSender);
	strTmp = MESSAGE_TO_SEND;
	for(i = 0; i < g_nTotalMsg; i++)
	{
		g_pStorageTest->SendMsg(Sender, strNumReceiver, strTmp, strMsgId);
		vMsgIds.push_back(strMsgId);
	}

	// Check messages for each device
	CNumDev Receiver = { strNumReceiver, strPinReceiver };
	unsigned int nTotal(0), nNew(0);
	CZorgeString strStatus, strTime, strF, strLoadedId;

	g_pStorageTest->OpenNumber(strNumReceiver, strPinReceiver);
	g_pStorageTest->GetMsgCount(Receiver, nTotal, nNew);
	if(g_nTotalMsg != nNew)
	{
		CZorgeString strErr;
		strErr.Format("Invalid message number. nTotalMsg=%d, nMsgNew=%d", g_nTotalMsg, nNew);
		throw CZorgeAssert(strErr);
	}

	// Check message ids
	TIds vIds;
	g_pStorageTest->GetIds(Receiver, MSG_ALL, vIds);
	if(vIds.size() != g_nTotalMsg)
	{
		CZorgeString strErr;
		strErr.Format("Invalid message number 1. vIds.size()=%d, nTotalMsg=%d", vIds.size(), g_nTotalMsg);
		throw CZorgeAssert(strErr);
	}

	TIds::const_iterator it = vIds.begin();
	for(; it != vIds.end(); ++it)
	{
		const CZorgeString& strLoadedLine = *(*it);
		ParseMessage(strLoadedLine, strStatus, strTime, strF, strLoadedId);

		CMessage* pMsg = g_pStorageTest->ReadMessage(Receiver, strLoadedId);
		if(!pMsg)
		{
			CZorgeString strErr;
			strErr.Format("Failed to get message. msgId=%s", (const char*)strLoadedId);
			throw CZorgeAssert(strErr);
		}

		if(strNumSender != pMsg->m_Record.m_Data.m_szFromNum)
		{
			CZorgeString strErr;
			strErr.Format("Wrong sender num. msgId=%s, strNumSender=%s, %s", (const char*)strLoadedId,
					(const char*)strNumSender, pMsg->m_Record.m_Data.m_szFromNum);
			throw CZorgeAssert(strErr);
		}

		if(strLoadedId != pMsg->m_Record.m_Data.m_szFileName)
		{
			CZorgeString strErr;
			strErr.Format("Wrong sender message id. msgId=%s, strLoadedLine=%s, %s", (const char*)strLoadedId,
					(const char*)strLoadedLine, pMsg->m_Record.m_Data.m_szFileName);
			throw CZorgeAssert(strErr);
		}

		if(pMsg->m_strText != MESSAGE_TO_SEND)
		{
			CZorgeString strErr;
			strErr.Format("Wrong sender message text. expected [%s], got [%s]", MESSAGE_TO_SEND, (const char*)pMsg->m_strText);
			throw CZorgeAssert(strErr);
		}
		delete pMsg;
	}
	releaseIds(vIds);
	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

#define MESSAGE_TO_SEND_CHIN "那些殺不死你的將使你更強大"
void TestMessageSend_UnicodeChin(const char* szFrom, const char* szTo)
{
	printf("--- TestMessageSend_UnicodeChin ");
	g_Timer.Start();

	CZorgeString strTmp, strDev;
	CZorgeString strNumSender(szFrom), strPinSender(szFrom);
	CZorgeString strNumReceiver(szTo), strPinReceiver(szTo);
	unsigned int i = 0;

	// Prepare number for sender
	if(g_pStorageTest->FindNumber(strNumSender))
		g_pStorageTest->DeleteNumber(strNumSender, strPinSender);
	g_pStorageTest->AddNumber(strNumSender, strPinSender, strTmp);

	// Prepare number for receiver
	if(g_pStorageTest->FindNumber(strNumReceiver))
		g_pStorageTest->DeleteNumber(strNumReceiver, strPinReceiver);
	g_pStorageTest->AddNumber(strNumReceiver, strPinReceiver, strTmp);

	// Send N messages
	vector<CZorgeString> vMsgIds;
	CZorgeString strMsgId;
	CNumDev Sender = { strNumSender, strPinSender};
	g_pStorageTest->OpenNumber(strNumSender, strPinSender);
	strTmp = MESSAGE_TO_SEND_CHIN;
	for(i = 0; i < g_nTotalMsg; i++)
	{
		g_pStorageTest->SendMsg(Sender, strNumReceiver, strTmp, strMsgId);
		vMsgIds.push_back(strMsgId);
	}

	// Check messages for each device
	CNumDev Receiver = { strNumReceiver, strPinReceiver };
	unsigned int nTotal(0), nNew(0);
	CZorgeString strStatus, strTime, strF, strLoadedId;

	g_pStorageTest->OpenNumber(strNumReceiver, strPinReceiver);

	g_pStorageTest->GetMsgCount(Receiver, nTotal, nNew);
	if(g_nTotalMsg != nNew)
	{
		CZorgeString strErr;
		strErr.Format("Invalid message number. nTotalMsg=%d, nMsgNew=%d", g_nTotalMsg, nNew);
		throw CZorgeAssert(strErr);
	}

	// Check message ids
	TIds vIds;
	g_pStorageTest->GetIds(Receiver, MSG_ALL, vIds);
	if(vIds.size() != g_nTotalMsg)
	{
		CZorgeString strErr;
		strErr.Format("Invalid message number 1. vIds.size()=%d, nTotalMsg=%d", vIds.size(), g_nTotalMsg);
		throw CZorgeAssert(strErr);
	}

	TIds::const_iterator it = vIds.begin();
	for(; it != vIds.end(); ++it)
	{
		const CZorgeString& strLoadedLine = *(*it);
		ParseMessage(strLoadedLine, strStatus, strTime, strF, strLoadedId);

		CMessage* pMsg = g_pStorageTest->ReadMessage(Receiver, strLoadedId);
		if(!pMsg)
		{
			CZorgeString strErr;
			strErr.Format("Failed to get message. msgId=%s", (const char*)strLoadedId);
			throw CZorgeAssert(strErr);
		}

		if(strNumSender != pMsg->m_Record.m_Data.m_szFromNum)
		{
			CZorgeString strErr;
			strErr.Format("Wrong sender num. msgId=%s, strNumSender=%s, %s", (const char*)strLoadedId,
					(const char*)strNumSender, pMsg->m_Record.m_Data.m_szFromNum);
			throw CZorgeAssert(strErr);
		}

		if(strLoadedId != pMsg->m_Record.m_Data.m_szFileName)
		{
			CZorgeString strErr;
			strErr.Format("Wrong sender message id. msgId=%s, strLoadedLine=%s, %s", (const char*)strLoadedId,
					(const char*)strLoadedLine, pMsg->m_Record.m_Data.m_szFileName);
			throw CZorgeAssert(strErr);
		}

		if(pMsg->m_strText != MESSAGE_TO_SEND_CHIN)
		{
			CZorgeString strErr;
			strErr.Format("Wrong sender message text. expected [%s], got [%s]", MESSAGE_TO_SEND, (const char*)pMsg->m_strText);
			throw CZorgeAssert(strErr);
		}
		delete pMsg;
	}
	releaseIds(vIds);
	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

#define MESSAGE_TO_SEND_RUS "Что не убьет тебя, то сделает тебя сильнее."
void TestMessageSend_UnicodeRUS(const char* szFrom, const char* szTo)
{
	printf("--- TestMessageSend_UnicodeRUS ");
	g_Timer.Start();

	CZorgeString strTmp, strDev;
	CZorgeString strNumSender(szFrom), strPinSender(szFrom);
	CZorgeString strNumReceiver(szTo), strPinReceiver(szTo);
	unsigned int i = 0;

	// Prepare number for sender
	if(g_pStorageTest->FindNumber(strNumSender))
		g_pStorageTest->DeleteNumber(strNumSender, strPinSender);
	g_pStorageTest->AddNumber(strNumSender, strPinSender, strTmp);

	// Prepare number for receiver
	if(g_pStorageTest->FindNumber(strNumReceiver))
		g_pStorageTest->DeleteNumber(strNumReceiver, strPinReceiver);
	g_pStorageTest->AddNumber(strNumReceiver, strPinReceiver, strTmp);

	// Send N messages
	vector<CZorgeString> vMsgIds;
	CZorgeString strMsgId;
	CNumDev Sender = { strNumSender, strPinSender };
	g_pStorageTest->OpenNumber(strNumSender, strPinSender);
	strTmp = MESSAGE_TO_SEND_RUS;
	for(i = 0; i < g_nTotalMsg; i++)
	{
		g_pStorageTest->SendMsg(Sender, strNumReceiver, strTmp, strMsgId);
		vMsgIds.push_back(strMsgId);
	}

	// Check messages for each device
	CNumDev Receiver = { strNumReceiver, strPinReceiver };
	unsigned int nTotal(0), nNew(0);
	CZorgeString strStatus, strTime, strF, strLoadedId;
	g_pStorageTest->OpenNumber(strNumReceiver, strPinReceiver);

	g_pStorageTest->GetMsgCount(Receiver, nTotal, nNew);
	if(g_nTotalMsg != nNew)
	{
		CZorgeString strErr;
		strErr.Format("Invalid message number. nTotalMsg=%d, nMsgNew=%d", g_nTotalMsg, nNew);
		throw CZorgeAssert(strErr);
	}

	// Check message ids
	TIds vIds;
	g_pStorageTest->GetIds(Receiver, MSG_ALL, vIds);
	if(vIds.size() != g_nTotalMsg)
	{
		CZorgeString strErr;
		strErr.Format("Invalid message number 1. vIds.size()=%d, nTotalMsg=%d", vIds.size(), g_nTotalMsg);
		throw CZorgeAssert(strErr);
	}

	TIds::const_iterator it = vIds.begin();
	for(; it != vIds.end(); ++it)
	{
		const CZorgeString& strLoadedLine = *(*it);
		ParseMessage(strLoadedLine, strStatus, strTime, strF, strLoadedId);

		CMessage* pMsg = g_pStorageTest->ReadMessage(Receiver, strLoadedId);
		if(!pMsg)
		{
			CZorgeString strErr;
			strErr.Format("Failed to get message. msgId=%s", (const char*)strLoadedId);
			throw CZorgeAssert(strErr);
		}

		if(strNumSender != pMsg->m_Record.m_Data.m_szFromNum)
		{
			CZorgeString strErr;
			strErr.Format("Wrong sender num. msgId=%s, strNumSender=%s, %s", (const char*)strLoadedId,
					(const char*)strNumSender, pMsg->m_Record.m_Data.m_szFromNum);
			throw CZorgeAssert(strErr);
		}

		if(strLoadedId != pMsg->m_Record.m_Data.m_szFileName)
		{
			CZorgeString strErr;
			strErr.Format("Wrong sender message id. msgId=%s, strLoadedLine=%s, %s", (const char*)strLoadedId,
					(const char*)strLoadedLine, pMsg->m_Record.m_Data.m_szFileName);
			throw CZorgeAssert(strErr);
		}

		if(pMsg->m_strText != MESSAGE_TO_SEND_RUS)
		{
			CZorgeString strErr;
			strErr.Format("Wrong sender message text. expected [%s], got [%s]", MESSAGE_TO_SEND, (const char*)pMsg->m_strText);
			throw CZorgeAssert(strErr);
		}
		delete pMsg;
	}
	releaseIds(vIds);
	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestBlocksCache()
{
	printf("--- TestBlocksCache ");
	g_Timer.Start();

	CZorgeString strNum, strPin, strNewNum;
	unsigned int nCurNum = g_nFromNum;
	unsigned int i = 0;
	for(i = 0; i < g_nTotalNumbers; ++i, ++nCurNum)
	{
		strNum.Format("%d", nCurNum);
		strPin = strNum;
		if(g_pStorageTest->FindNumber(strNum))
			g_pStorageTest->DeleteNumber(strNum, strPin);
		g_pStorageTest->AddNumber(strNum, strPin, strNewNum);
	}

	// Send itself a message
	CZorgeString strTmp("test message"), strMsgId;
	nCurNum = g_nFromNum;
	for(i = 0; i < g_nTotalNumbers; i++, nCurNum++)
	{
		strNum.Format("%d", nCurNum);
		strPin = strNum;
		g_pStorageTest->OpenNumber(strNum, strPin);
		CNumDev Sender = { strNum, strPin };
		g_pStorageTest->SendMsg(Sender, strNum, strTmp, strMsgId);
	}

	unsigned int nTotal(0), nNew(0);
	nCurNum = g_nFromNum;
	for(i = 0; i < g_nTotalNumbers; i++, nCurNum++)
	{
		strNum.Format("%d", nCurNum);
		strPin = strNum;
		g_pStorageTest->OpenNumber(strNum, strPin);
		CNumDev Sender = { strNum, strPin };
		g_pStorageTest->GetMsgCount(Sender, nTotal, nNew);
	}

	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestMessageRead(const char* szFrom, const char* szTo)
{
	printf("--- TestMessageRead ");
	g_Timer.Start();

	CZorgeString strTmp, strDev;
	CZorgeString strNumSender(szFrom), strPinSender(szFrom);
	CZorgeString strNumReceiver(szTo), strPinReceiver(szTo);
	unsigned int i = 0;

	// Prepare number for sender
	if(g_pStorageTest->FindNumber(strNumSender))
		g_pStorageTest->DeleteNumber(strNumSender, strPinSender);
	g_pStorageTest->AddNumber(strNumSender, strPinSender, strTmp);

	// Prepare number for receiver
	if(g_pStorageTest->FindNumber(strNumReceiver))
		g_pStorageTest->DeleteNumber(strNumReceiver, strPinReceiver);
	g_pStorageTest->AddNumber(strNumReceiver, strPinReceiver, strTmp);

	// Send N messages
	vector<CZorgeString> vMsgIds;
	CZorgeString strMsgId;
	CNumDev Sender = { strNumSender, strPinSender };
	g_pStorageTest->OpenNumber(strNumSender, strPinSender);
	strTmp = MESSAGE_TO_SEND;
	for(i = 0; i < g_nTotalMsg; i++)
	{
		g_pStorageTest->SendMsg(Sender, strNumReceiver, strTmp, strMsgId);
		vMsgIds.push_back(strMsgId);
	}

	// Check messages for each device
	CNumDev Receiver = { strNumReceiver, strPinReceiver };
	unsigned int nMsgTotal(0), nMsgNew(0);
	CZorgeString strStatus, strTime, strF, strLoadedId;

	g_pStorageTest->OpenNumber(strNumReceiver, strPinReceiver);
	g_pStorageTest->GetMsgCount(Receiver, nMsgTotal, nMsgNew);
	if(g_nTotalMsg != nMsgTotal && nMsgTotal != nMsgNew)
	{
		CZorgeString strErr;
		strErr.Format("Invalid message number. nTotalMsg=%d, nMsgTotal=%d, nMsgNew=%d", g_nTotalMsg, nMsgTotal, nMsgNew);
		throw CZorgeAssert(strErr);
	}

	// Check read message
	TIds vIds;
	g_pStorageTest->GetIds(Receiver, MSG_NEW, vIds);
	size_t nOrigSize = vIds.size();
	if(nOrigSize != g_nTotalMsg)
	{
		CZorgeString strErr;
		strErr.Format("Invalid message number (1). nTotalMsg=%d, nMsgTotal=%d, nMsgNew=%d", g_nTotalMsg, nMsgTotal, nMsgNew);
		throw CZorgeAssert(strErr);
	}

	while(nOrigSize > 0)
	{
		const CZorgeString& strTop = *(*vIds.begin());
		ParseMessage(strTop, strStatus, strTime, strF, strLoadedId);
		CMessage* pMsg = g_pStorageTest->ReadMessage(Receiver, strLoadedId);
		if(!pMsg)
		{
			CZorgeString strErr;
			strErr.Format("Failed to get message. msgId=%s", (const char*)strLoadedId);
			throw CZorgeAssert(strErr);
		}
		delete pMsg;

		releaseIds(vIds);

		g_pStorageTest->GetIds(Receiver, MSG_NEW, vIds);
		size_t nCurSize = vIds.size();
		if(nOrigSize - nCurSize != 1)
		{
			CZorgeString strErr;
			strErr.Format("Failed point 1. nOrigSize=%ld, nCurSize=%ld", nOrigSize, nCurSize);
			throw CZorgeAssert(strErr);
		}
		nOrigSize = nCurSize;
	}
	releaseIds(vIds);
	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestMessageDelete(const char* szFrom)
{
	printf("--- TestMessageDelete ");
	g_Timer.Start();

	CZorgeString strTmp, strDev;
	CZorgeString strNumSender(szFrom), strPinSender(szFrom);
	unsigned int i = 0;

	// Prepare number for sender
	if(g_pStorageTest->FindNumber(strNumSender))
		g_pStorageTest->DeleteNumber(strNumSender, strPinSender);
	g_pStorageTest->AddNumber(strNumSender, strPinSender, strTmp);

	// Send N messages
	vector<CZorgeString> vMsgIds;
	CZorgeString strMsgId;
	CNumDev Sender = { strNumSender, strPinSender };
	g_pStorageTest->OpenNumber(strNumSender, strPinSender);
	strTmp = MESSAGE_TO_SEND;
	for(i = 0; i < g_nTotalMsg; i++)
	{
		g_pStorageTest->SendMsg(Sender, strNumSender, strTmp, strMsgId);
		vMsgIds.push_back(strMsgId);
	}

	// Check messages for each device
	unsigned int nMsgTotal(0), nMsgNew(0);
	CZorgeString strStatus, strTime, strF, strLoadedId;
	g_pStorageTest->OpenNumber(strNumSender, strPinSender);
	g_pStorageTest->GetMsgCount(Sender, nMsgTotal, nMsgNew);
	if(g_nTotalMsg != nMsgTotal && nMsgTotal != nMsgNew)
	{
		CZorgeString strErr;
		strErr.Format("Invalid message number. nTotalMsg=%d, nMsgTotal=%d, nMsgNew=%d", g_nTotalMsg, nMsgTotal, nMsgNew);
		throw CZorgeAssert(strErr);
	}

	// Delete messages one by one
	TIds vIds;
	g_pStorageTest->GetIds(Sender, MSG_NEW, vIds);
	size_t nOrigSize = vIds.size();
	if(nOrigSize != g_nTotalMsg)
	{
		CZorgeString strErr;
		strErr.Format("Invalid message number (1). nTotalMsg=%d, nMsgTotal=%d, nMsgNew=%d", g_nTotalMsg, nMsgTotal, nMsgNew);
		throw CZorgeAssert(strErr);
	}

	while(nOrigSize > 0)
	{
		const CZorgeString& strTop = *(*vIds.begin());
		ParseMessage(strTop, strStatus, strTime, strF, strLoadedId);
		g_pStorageTest->DeleteMessage(Sender, strLoadedId);

		releaseIds(vIds);
		g_pStorageTest->GetIds(Sender, MSG_NEW, vIds);
		size_t nCurSize = vIds.size();
		if(nOrigSize - nCurSize != 1)
		{
			CZorgeString strErr;
			strErr.Format("Failed point 1. nOrigSize=%ld, nCurSize=%ld", nOrigSize, nCurSize);
			throw CZorgeAssert(strErr);
		}
		nOrigSize = nCurSize;
	}

	releaseIds(vIds);
	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestMessageCompression(const char* szFrom)
{
	//int nCount1 = (int)GetRandomByte();
	//int nCount2 = (int)GetRandomByte();

	int nCount1 = 500;
	int nCount2 = 500;

	if(nCount1 == 0)
		nCount1 = 10;
	if(nCount2 == 0)
		nCount2 = 10;

	printf("--- TestMessageCompression (count1 = %d, count2 = %d) ", nCount1, nCount2);
	g_Timer.Start();

	CZorgeString strTmp, strDev("000"), strMsgId;
	CZorgeString strNumSender(szFrom), strPinSender(szFrom);

	// Prepare number for sender
	if(g_pStorageTest->FindNumber(strNumSender))
		g_pStorageTest->DeleteNumber(strNumSender, strPinSender);
	g_pStorageTest->AddNumber(strNumSender, strPinSender, strTmp);

	CNumDev Sender = { strNumSender, strPinSender };
	TIds vIds;
	typedef map<CZorgeString, CZorgeString> TMsgs;
	TMsgs mMsg;
	CZorgeString strStatus, strTime, strF, strLoadedId;
	for(int i = 0; i < nCount1; ++i)
	{
		mMsg.clear();
		// Send N messages to myself
		for(int j = 0; j < nCount2; ++j)
		{
			strTmp.Format("Testing message %d, %d", i, j);
			g_pStorageTest->SendMsg(Sender, strNumSender, strTmp, strMsgId);
			mMsg.insert(make_pair(strMsgId, strTmp));
		}

		releaseIds(vIds);
		g_pStorageTest->GetIds(Sender, MSG_NEW, vIds);
		if(vIds.size() == 0)
			throw CZorgeAssert("Assert(vIds.size() == 0)");

		// Read and delete all messages
		for(CZorgeString* p : vIds)
		{
			const CZorgeString& strLine = *p;
			ParseMessage(strLine, strStatus, strTime, strF, strLoadedId);
			CMessage* pMsg = g_pStorageTest->ReadMessage(Sender, strLoadedId);
			TMsgs::iterator it = mMsg.find(strLoadedId);
			if(it == mMsg.end())
			{
				delete pMsg;
				strTmp.Format("Message not found, id = %s", (const char*)strLoadedId);
				throw CZorgeAssert(strTmp);
			}
			if(pMsg->m_strText != (*it).second)
			{
				delete pMsg;
				strTmp.Format("Message content check failed, id = %s, [%s] != [%s]", (const char*)strLoadedId, (const char*)pMsg->m_strText, (const char*)(*it).second);
				throw CZorgeAssert(strTmp);
			}
			delete pMsg;
			g_pStorageTest->DeleteMessage(Sender, strLoadedId);
		}

		releaseIds(vIds);
	}

	float fMsgPerSecond = (float)(nCount1 * nCount2)/g_Timer.GetMilliSecs();
	printf("%0.1f iter/sec, %s\n", fMsgPerSecond * 1000.0, g_Timer.ToString(g_szTimer, 64));
}

void TestMessageCompressionOnAdd(const char* szFrom)
{
	printf("--- TestMessageCompressionOnAdd ");
	g_Timer.Start();

	CZorgeString strTmp, strMsgId;
	CZorgeString strNumSender(szFrom), strPinSender(szFrom);

	// Prepare number for sender
	if(g_pStorageTest->FindNumber(strNumSender))
		g_pStorageTest->DeleteNumber(strNumSender, strPinSender);
	g_pStorageTest->AddNumber(strNumSender, strPinSender, strTmp);

	CNumDev Sender = { strNumSender, strPinSender };
	// Send N messages to myself
	for(int j = 0; j < 1024; ++j)
	{
		strTmp.Format("Testing message %d", j);
		g_pStorageTest->SendMsg(Sender, strNumSender, strTmp, strMsgId);
	}

	CZorgeString strStatus, strTime, strF, strLoadedId;
	TIds vIds;
	g_pStorageTest->GetIds(Sender, MSG_NEW, vIds);
	if(vIds.size() != 1024)
	{
		strTmp.Format("Invalid size() %ld != %ld", vIds.size(), 1024);
		throw CZorgeAssert(strTmp);
	}

	// Delete one message
	const CZorgeString& strLine = **vIds.begin();
	ParseMessage(strLine, strStatus, strTime, strF, strLoadedId);
	g_pStorageTest->DeleteMessage(Sender, strLoadedId);

	// Add message to trigger compression
	strTmp = "Testing message with compression";
	g_pStorageTest->SendMsg(Sender, strNumSender, strTmp, strMsgId);

	// Find message by id
	CMessage* p = g_pStorageTest->ReadMessage(Sender, strMsgId);
	if(p->m_strText != strTmp)
	{
		strTmp.Format("Invalid text [%s] != [%s]", (const char*)p->m_strText, (const char*)strTmp);
		throw CZorgeAssert(strTmp);
	}
	delete p;
	releaseIds(vIds);

	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestIncrementNumberLength()
{
	printf("--- TestIncrementNumberLength ");
	g_Timer.Start();
	CZorgeString strNum("99"), strPin, strNewNum;
	unsigned int nLen = strNum.GetLength();

	// Create targets
	vector<CZorgeString> vNums;
	for(; nLen < MAX_DIGITS_IN_NUMBER; ++nLen)
	{
		strNum += "9"; strPin = strNum;

		if(g_pStorageTest->FindNumber(strNum))
			g_pStorageTest->DeleteNumber(strNum, strPin);

		g_pStorageTest->AddNumber(strNum, strPin, strNewNum);
		vNums.push_back(strNewNum);
	}

	// Create source
	strNum = "101"; strPin = strNum;
	if(g_pStorageTest->FindNumber(strNum))
		g_pStorageTest->DeleteNumber(strNum, strPin);
	g_pStorageTest->AddNumber(strNum, strPin, strNewNum);

	// Send one message to each target number
	CZorgeString strMsgId;
	CNumDev From = {strNum, strPin};
	typedef map<CZorgeString, char> TMapIds;
	TMapIds mIds;
	for(CZorgeString strToNum : vNums)
	{
		g_pStorageTest->SendMsg(From, strToNum, strToNum, strMsgId);
		mIds.insert(make_pair(strMsgId, 0));
	}

	// Read all messages
	unsigned int nTotal(0), nNew(0);
	TIds vOutIds;
	CZorgeString strStatus, strTime, strFrom, strId;
	for(CZorgeString strToNum : vNums)
	{
		CNumDev Check = {strToNum, strToNum};
		// Assert only one new message
		g_pStorageTest->GetMsgCount(Check, nTotal, nNew);
		if(nNew != 1)
			throw CZorgeAssert("Expected one message. Point 1");

		// Get ids
		g_pStorageTest->GetIds(Check, MSG_NEW, vOutIds); // leak on exception
		if(vOutIds.size() != 1)
			throw CZorgeAssert("Expected one message. Point 2");

		CZorgeString& strLine = *vOutIds[0];
		ParseMessage(strLine, strStatus, strTime, strFrom, strId);

		// Assert id was send
		TMapIds::iterator it = mIds.find(strId);
		if(it == mIds.end())
			throw CZorgeAssert("Message was not send.");
		mIds.erase(it);

		// Read message and check content
		CMessage* pMsg = g_pStorageTest->ReadMessage(Check, strId);
		if(pMsg->m_strText != strToNum)
			throw CZorgeAssert("Message text is invalid.");
		if(strcmp(pMsg->m_Record.m_Data.m_szFromNum, "101") != 0)
			throw CZorgeAssert("Message from number is invalid.");
		delete pMsg;

		// Check that message marked as read.
		g_pStorageTest->GetMsgCount(Check, nTotal, nNew);
		if(nNew != 0)
			throw CZorgeAssert("Expected no messages.");

		// Release memory
		for(CZorgeString* p : vOutIds)
			delete p;
		vOutIds.clear();

	}
	if(mIds.size() != 0)
		throw CZorgeAssert("Not all messages read.");

	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestToken(const char* szFrom)
{
	printf("--- TestToken ");
	g_Timer.Start();

	CZorgeString strTmp;
	CZorgeString strNum(szFrom), strPin(szFrom);

	// Prepare number for sender
	if(g_pStorageTest->FindNumber(strNum))
		g_pStorageTest->DeleteNumber(strNum, strPin);
	g_pStorageTest->AddNumber(strNum, strPin, strTmp);

	CNumDev Num = { strNum, strPin };
	CZorgeString strToken1, strToken2;
	for(int i = 0; i < 100; ++i)
	{
		strToken1 = g_pStorageTest->NewNumToken(Num);
		if(strToken1.IsEmpty())
			throw CZorgeAssert("Failed to get new token");

		CNumDev Num1 = { strNum, strToken1 };
		if(!g_pStorageTest->CheckNumToken(Num1))
			throw CZorgeAssert("Check token failed. 1");

		Num1.m_strPin += "invalid";
		if(g_pStorageTest->CheckNumToken(Num1))
			throw CZorgeAssert("Check token failed. 2");

		strToken2 = g_pStorageTest->GetNumToken(Num);
		if(strToken1 != strToken2)
			throw CZorgeAssert("Failed to get token");

		g_pStorageTest->DeleteNumToken(Num);
		strToken2 = g_pStorageTest->GetNumToken(Num);
		if(!strToken2.IsEmpty())
			throw CZorgeAssert("Failed to delete token");
	}

	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void RootForTest(const CZorgeString& strMainRoot, CZorgeString& strTestRoot)
{
	strTestRoot = strMainRoot;
	if(strTestRoot.IsLastChar('/'))
		strTestRoot.Chop();

	int nPos = 0;
	if((nPos = strTestRoot.ReverseFind('/')) == STR_NOT_FOUND)
		throw CZorgeError("Invalid root path.");
	strTestRoot = strTestRoot.Left(nPos);
	strTestRoot += "/zsms_storage_test";
}

int Test(const CZorgeString& strMainRoot, int nTotalIters)
{
	try
	{
		printf("--------------------------- Test. Iterations = %d\n", nTotalIters);
		if(nTotalIters < 1)
			throw CZorgeError("Invalid iteration number");

		CZorgeString strRoot;
		CZorgeString strRepoKey = "key";

		RootForTest(strMainRoot, strRoot);

		printf("g_nTotalNumbers = %d\n", g_nTotalNumbers);
		printf("g_nFromNum = %d\n", g_nFromNum);
		printf("g_nTotalMsg = %d\n", g_nTotalMsg);

		MkDir(strRoot);

		g_pStorageTest = new CStorage();
		g_pStorageTest->SetRoot(strRoot);
		printf("Root : %s\n", (const char*)strRoot);
		printf("Key : %s\n\n", (const char*)strRepoKey);

		if(g_pStorageTest->GetStatus() == CStorage::ReadyToOpen)
		{
			try
			{
				g_pStorageTest->Login(strRepoKey);
			}
			catch(CZorgeError&)
			{
				if(g_pStorageTest->GetStatus() != CStorage::NotInit)
					throw;
			}
			puts("Init repo");
			g_pStorageTest->Init(strRepoKey);
		}
		else
		{
			puts("Init repo");
			g_pStorageTest->Init(strRepoKey);
		}

		typedef tuple<unsigned long, CZorgeString> TTuple;
		vector<TTuple> vLog;
		unsigned long nPrev(0), nDif(0);
		CZorgeString str;
		for(int i = 0; i < nTotalIters; ++i)
		{
			unsigned long nMem = GetResMemUsage_KB();
			nDif = nMem - nPrev;
			nPrev = nMem;

			printf("==> iteration %d/%d %ld KB, dif = %ld KB\n", i+1, nTotalIters, nMem, nDif);
			for(auto t : vLog)
				printf("%ld ", get<0>(t));
			puts("");

			CZorgeTimer Timer;
			//----------------------------------------------------------
			// Positive tests for numbers
			//----------------------------------------------------------
			TestChangePin("303");
			TestOpenNumbers();
			TestDeleteNumbers();
			TestOpenNumbers();
			TestIncrementNumberLength();

			//----------------------------------------------------------
			// Positive tests for messages
			//----------------------------------------------------------
			TestMessageSend("101", "102");
			TestMessageSend("101", "101");
			TestMessageSend_UnicodeRUS("101", "101");
			TestMessageSend_UnicodeRUS("101", "102");
			TestMessageSend_UnicodeChin("101", "101");
			TestMessageSend_UnicodeChin("101", "102");
			TestMessageRead("101", "101");
			TestMessageRead("101", "102");
			TestMessageDelete("101");
			TestToken("101");
			TestMessageCompression("303");

			TestMessageCompressionOnAdd("404");
			//----------------------------------------------------------
			// Other tests
			//----------------------------------------------------------
			TestBlocksCache();

			//----------------------------------------------------------
			// Invalid tests for numbers
			//----------------------------------------------------------
			TestDeleteNumbers_Invalid();
			TestOpenNumbers_Invalid();
			TestMessageSend_Invalid();
			TestMessageRead_Invalid();
			TestMessageDelete_Invalid();
			TestChangePin_Invalid();

			g_pStorageTest->Diagnostics(str);
			printf("%s\n", (const char*)str);

			//----------------------------------------------------------
			// Threads.
			//----------------------------------------------------------
			Test_Thread(g_pStorageTest, false);

			Timer.Stop();

			//g_pStorageTest->Login(strRepoKey);
			nMem = GetResMemUsage_KB();
			printf("--------------------------- Test time %s, mem=%ld KB\n", Timer.ToString(g_szTimer, 64), nMem);
			vLog.push_back(make_tuple(nMem, CZorgeString(g_szTimer)));
		}
		// print results
		{
			puts("====> Log");
			unsigned long nPrev = 0, nCur(0), nDif(0);
			for(auto t : vLog) {
				nCur = get<0>(t);
				if(nPrev)
					nDif = nCur - nPrev;
				printf("%ld KB %s %ld KB\n", nCur, (const char*)get<1>(t), nDif);
				nPrev = nCur;
			}
			puts("");
		}
	}
	catch(ALL_ERRORS& e)
	{
		printf("Error = %s\n", e.what());
	}

	puts("-------------- End of test\n\n\n");
	delete g_pStorageTest;
	g_pStorageTest = 0;
	return 0;
}
