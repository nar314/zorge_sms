#include <stdio.h>

#include "ZorgeTimer.h"
#include "ZorgeThread.h"
#include "SysUtils.h"
#include "StrUtils.h"
#include "Storage.h"

static bool g_bPrint = false;
CStorage* g_pStorage1 = 0;
atomic<int> g_nWriters = 0;
atomic<int> g_nReaders = 0;
atomic<int> g_nNoises = 0;
static int g_nMax = 100000;

atomic<unsigned long> g_nMsgCount = 0;
atomic<bool> g_bStop = false;

static CZorgeString g_strMsgAppend(" ");

static void* Noise(void*)
{
	CSetSignalHandler SignaHandler;
	long nThreadId = (long)pthread_self();
	g_nNoises++;

	unsigned long long nTotalCalls = 0;
	try
	{
		CZorgeString strNum = "101";
		CZorgeString strMsg, strMsgId;
		CNumDev OpenNum = {strNum, strNum};

		while(g_bStop == false)
		{
			ZorgeSleep(0);
			TIds vIds;
			g_pStorage1->GetIds(OpenNum, MSG_NEW, vIds);
			nTotalCalls++;
			//if(g_bPrint)
				//printf("%lx messages %ld\n", nThreadId, vIds.size());
			for(CZorgeString* strId : vIds)
				delete strId;
		}
	}
	catch(ALL_ERRORS& e)
	{
		printf("[%lX] Noise error. %s\n", nThreadId, e.what());
	}
	g_nNoises--;
	int nLeft = (int)g_nNoises;
	if(g_bPrint)
		printf("[%lX] Noise left : %d, total calls by this thread is %lld\n", nThreadId, nLeft, nTotalCalls);

	return 0;
}

static void* SendMsg(void*)
{
	CSetSignalHandler SignaHandler;
	unsigned long nThreadId = (long)pthread_self();

	try
	{
		CZorgeString strNum = "101";
		const CZorgeString strToNum = strNum;
		CZorgeString strMsg, strMsgId;
		CNumDev OpenNum = {strNum, strNum};

		int i = 0;
		for(; i < g_nMax; ++i)
		{
			bool bPrint = (i % 5000) == 0;
			if(!g_bPrint)
				bPrint = false;
			strMsg.Format("Message %d", i);
			strMsg += g_strMsgAppend;
			//printf("%s\n", (const char*)strMsg);
			while(true)
			{
				try
				{
					g_pStorage1->SendMsg(OpenNum, strToNum, strMsg, strMsgId);
					g_nMsgCount++;
					if(bPrint)
						printf("[%lX] Writer. %d / %d \n", nThreadId, i, g_nMax);
					break;
				}
				catch(CZorgeError& e)
				{
					if(e.GetMsg().Find("No space for new message") != -1)
					{
						//printf("retry [%s]\n", (const char*)strMsg);
						ZorgeSleep(100);
					}
					else
					{
						printf("[%lX] Writer got error %s\n", nThreadId, (const char*)e.GetMsg());
						throw;
					}
				}
			}
		}
	}
	catch(ALL_ERRORS& e)
	{
		printf("[%lX] SendMsg error. %s\n", nThreadId, e.what());
	}
	g_nWriters--;
	int nLeft = (int)g_nWriters;
	if(g_bPrint)
		printf("[%lX] Writers left : %d\n", nThreadId, nLeft);

	return 0;
}

extern void ParseMessage(const CZorgeString&, CZorgeString&, CZorgeString&, CZorgeString&, CZorgeString&);

static void* ReadMsg(void*)
{
	CSetSignalHandler SignaHandler;
	long nThreadId = (long)pthread_self();

	try
	{
		CZorgeString strNum = "101";
		const CZorgeString strToNum = strNum;
		CZorgeString strMsg, strMsgId;
		CNumDev OpenNum = {strNum, strNum};

		CZorgeString strStatus, strTime, strF, strLoadedId;
		int i = 0;
		while(true)
		{
			TIds vIds;
			g_pStorage1->GetIds(OpenNum, MSG_NEW, vIds);
			if(vIds.size() == 0)
			{
				if((int)g_nWriters == 0)
					break;
				ZorgeSleep(10);
				continue;
			}
			CMessage* pMsg = 0;
			for(CZorgeString* strId : vIds)
			{
				++i;
				bool bPrint = (i % 5000) == 0;
				if(!g_bPrint)
					bPrint = false;
				try
				{
					ParseMessage(*strId, strStatus, strTime, strF, strLoadedId);
					pMsg = 0;
					pMsg = g_pStorage1->ReadMessage(OpenNum, strLoadedId);
					if(bPrint)
						printf("\t[%lX] Reader got [%s]\n", nThreadId, (const char*)pMsg->m_strText.Left(16));

					if(g_pStorage1->DeleteMessage(OpenNum, strLoadedId) == MSG_DELETED)
						g_nMsgCount--;
				}
				catch(CZorgeError& e)
				{
					CZorgeString s = e.GetMsg();
					if(s.Find("Message not found") == -1 && s.Find("Message deleted.") == -1)
						throw;
				}
				delete pMsg;
				delete strId;
			}
		}
	}
	catch(ALL_ERRORS& e)
	{
		printf("[%lX] ReadMsg error. %s\n", nThreadId, e.what());
	}
	g_nReaders--;
	int nLeft = (int)g_nReaders;
	if(g_bPrint)
		printf("[%lX] Readers left : %d\n", nThreadId, nLeft);

	return 0;
}

static void PrepareEnv()
{
	CZorgeString strNum("101"), strPin("101"), strTmp;

	if(g_pStorage1->FindNumber(strNum))
		g_pStorage1->DeleteNumber(strNum, strPin);
	g_pStorage1->AddNumber(strNum, strPin, strTmp);
}

int Test_Thread(CStorage* pStorage, bool bPrint)
{
	try
	{
		g_bPrint = bPrint;
		g_strMsgAppend += g_strMsgAppend;
		if(g_strMsgAppend.GetLength() >= 1024 * 4)
		{
			printf("Message length is %d > 4K. Forcing to 1 byte. \n", g_strMsgAppend.GetLength());
			g_strMsgAppend = " ";
		}

		g_nMsgCount = 0;
		g_bStop = false;
		g_pStorage1 = pStorage;
		int nWritersStarted = 5;
		int nReadersStarted = 5;
		int nTotalNoises = 10;
		g_nWriters = nWritersStarted;
		g_nReaders = nReadersStarted;

		printf("Prepend : %d bytes\n", g_strMsgAppend.GetLength());
		printf("Readers : %d\n", nReadersStarted);
		printf("Writers : %d\n", nWritersStarted);
		printf("g_nMax = %d, total messages to send %d\n", g_nMax, g_nMax * nWritersStarted);

		PrepareEnv();

		CZorgeTimer t;
		size_t nStack64K = 1024 * 64;
		t.Start();

		CZorgeThread r1(nStack64K), r2(nStack64K), r3(nStack64K), r4(nStack64K), r5(nStack64K);
		CZorgeThread w1(nStack64K), w2(nStack64K), w3(nStack64K), w4(nStack64K), w5(nStack64K);

		r1.Start(ReadMsg);
		r2.Start(ReadMsg);
		r3.Start(ReadMsg);
		r4.Start(ReadMsg);
		r5.Start(ReadMsg);

		w1.Start(SendMsg);
		w2.Start(SendMsg);
		w3.Start(SendMsg);
		w4.Start(SendMsg);
		w5.Start(SendMsg);

		vector<CZorgeThread*> vNoises;
		for(int i = 0; i < nTotalNoises; ++i)
		{
			CZorgeThread* pThread = new CZorgeThread(nStack64K);
			vNoises.push_back(pThread);
			pThread->Start(Noise);
		}

		printf("Noises thread started : %ld\n", vNoises.size());

		while((int)g_nWriters != 0)
			ZorgeSleep(100);

		puts("+++++++++++++++++ waiting for readers.");
		while(g_nReaders != 0)
			ZorgeSleep(100);

		t.Stop();
		char szTimer[64];
		printf("%s\n", t.ToString(szTimer, 64));

		g_bStop = true;
		puts("+++++++++++++++++ waiting for noises.");
		while((int)g_nNoises != 0)
			ZorgeSleep(100);

		for(CZorgeThread* p : vNoises)
			delete p;
		vNoises.clear();

		long nSecs = t.GetSecs();
		printf("nSecs = %ld, g_nMax = %d, nWritersStarted = %d, sent = %d\n", nSecs, g_nMax, nWritersStarted, g_nMax * nWritersStarted);
		if(nSecs != 0)
		{
			double msgPerSecond = (g_nMax * nWritersStarted) / nSecs;
			printf("msgPerSecond = %lf\n\n", msgPerSecond);
		}
		printf("g_nMsgCount = %ld, expecting 0\n", (unsigned long)g_nMsgCount);
		printf("Readers : %d\n", nReadersStarted);
		printf("Writers : %d\n", nWritersStarted);
		printf("g_nMax = %d, total messages to send %d\n", g_nMax, g_nMax * nWritersStarted);

		if(g_nMsgCount != 0)
			return 1;
		return 0;
	}
	catch(ALL_ERRORS& e)
	{
		printf("Error = %s\n", e.what());
		return 1;
	}
	return 0;
}

