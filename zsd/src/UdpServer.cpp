#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h> // close
#include <stdlib.h> // atoi

#include <atomic>

#include "ZorgeError.h"
#include "ZorgeMutex.h"
#include "SysUtils.h"

#include "UdpServer.h"
#include "Keys.h"
#include "CommandId.h"

#define PORT_NOT_SET 				0
#define REQUEST_BUFFER_SIZE			65535 // 64K
#define DEFAULT_SOCKET_POOL_SIZE 	2
#define MAX_QUEUE_ITEMS_COUNT 		30000
#define DEFAULT_THEADS 				10
#define DEFAULT_MON_TIMOUT 			5

static bool g_bReplyTraceOn = false;
static int g_nUdpMessageSize = sizeof(CUdpMessage);

static void* ProcFunc(void* pParam);
static void* ProcMonitorFunc(void* pParam);

atomic<unsigned int> g_nRunningThreadCount = 0;
atomic<bool> g_bMonitorRunning = false;

static CKeys g_Keys;

atomic<unsigned int> g_CUdpRequestItem = 0;
atomic<unsigned int> g_CUdpRequestItem_max = 0;

void DiagCUdpRequestItem(CZorgeString& strOut)
{
	CZorgeString str;
	str.Format("CUdpRequestItem.Cur=%d, Max=%d\n", (unsigned int)g_CUdpRequestItem, (unsigned int)g_CUdpRequestItem_max);
	strOut += str;
}

struct CUdpRequestItemCounter
{
	~CUdpRequestItemCounter()
	{
		printf("g_CUdpRequestItem = %u (%u)\n", (unsigned int)g_CUdpRequestItem, (unsigned int)g_CUdpRequestItem_max);
	}
} g_CUdpRequestItemCounter;

CUdpRequestItem::CUdpRequestItem()
{ // m_Client has garbage. Yep.
	m_nDataSize = 0;
	m_pData = 0;
	//m_szCallerIp[0] = 0;
	g_CUdpRequestItem++;
	g_CUdpRequestItem_max++;
}

void CUdpRequestItem::Print() const
{
	printf("m_nDataSize = %d\n", m_nDataSize);
}

CUdpRequestItem::~CUdpRequestItem()
{
	g_CUdpRequestItem--;
	if(m_pData)
		delete [] m_pData;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CUdpServer::CUdpServer(int nPort, unsigned int nThreads)
{
	m_pCoderRSA = new CCoderRSA();
	m_pCoderRSA->Init();
	m_pCoderAES = new CCoderAES(COM_ENCODER);

	m_pStorage = 0;
	m_nMonTimeOutSec = DEFAULT_MON_TIMOUT;
	m_nThreads = nThreads < 1 ? DEFAULT_THEADS : nThreads;
	m_nMonitorCallsCount = 0;
	m_pMonitor = 0;
	m_Mutex = PTHREAD_MUTEX_INITIALIZER;
	m_Cond = PTHREAD_COND_INITIALIZER;

	m_nRequestBufferSize = REQUEST_BUFFER_SIZE;
	m_pRequestBuffer = new unsigned char[m_nRequestBufferSize];
	m_bRunning = false;
	m_bShutDown = false;
	m_Socket = -1;

	if(nPort > 0)
		m_nPort = nPort;
	else
	{
		m_nPort = PORT_NOT_SET;
		printf("Assert(true) Invalid server port %d\n", nPort);
	}
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CUdpServer::~CUdpServer()
{
	unsigned int n = g_nRunningThreadCount;
	if(n > 0)
	{
		printf("Waiting for threads to exit. Running : %d\n", n);
		// 5 seconds
		for(int i = 0; i < 500; ++i)
		{
			ZorgeSleep(100);
			n = g_nRunningThreadCount;
			if(n == 0)
				break;
		}
		if(n > 0)
			printf("Still running threads : %d\n", n);
	}

	bool b = g_bMonitorRunning;
	if(b)
	{
		puts("Waiting for monitor thread to exit.");
		// 5 seconds
		for(int i = 0; i < 500; ++i)
		{
			ZorgeSleep(100);
			b = g_bMonitorRunning;
			if(b == 0)
				break;
		}
		if(b)
			puts("Monitor thread still running.");
	}

	TThreads::iterator it = m_vThreads.begin();
	for(; it != m_vThreads.end(); ++it)
		delete *it;
	m_vThreads.clear();

	if(m_pMonitor)
		delete m_pMonitor;

	if(m_Socket != -1)
		close(m_Socket);

	if(m_pRequestBuffer)
		delete [] m_pRequestBuffer;

	if(m_pCoderRSA)
		delete m_pCoderRSA;

	if(m_pCoderAES)
		delete m_pCoderAES;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CUdpServer::SetMonitorTimeout(unsigned int nTimeOut)
{
	if(nTimeOut < 1)
	{
		m_nMonTimeOutSec = DEFAULT_MON_TIMOUT;
		printf("Invalid value for timeout. Forcing default value %d sec.\n", m_nMonTimeOutSec);
	}
	else
		m_nMonTimeOutSec = nTimeOut;
}

#define _24Hours 86400 // in seconds
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CUdpServer::SetStorage(CStorage* p)
{
	if(p == 0)
		throw CZorgeError("Assert(true) CUdpServer::SetStorage");
	m_pStorage = p;

	CConfig& Config = GetConfig();
	CZorgeString strValue;
	if(Config.GetValue(CONFIG_REPLY_TRACE, strValue))
	{
		g_bReplyTraceOn = strValue == "1";
		m_OpenConsCache.SetTrace(g_bReplyTraceOn);
	}

	if(Config.GetValue(CONFIG_REPLY_CHACHE_TTL, strValue))
	{
		unsigned int n = atoi((const char*)strValue);
		if(n > 1 && n < _24Hours)
			m_OpenConsCache.SetTTL(n);
		else
			printf("%s has invalid value %s\n", CONFIG_REPLY_CHACHE_TTL, (const char*)strValue);
	}

}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CUdpServer::Run()
{
	if(m_pStorage == 0)
		throw CZorgeError("Storage not set.");
	if(m_Socket != -1)
		throw CZorgeError("Server can not be started. Socket not allocated.");
	if(m_nPort == PORT_NOT_SET)
		throw CZorgeError("Server port not set.");

	printf("pid %d\n", getpid());
	//---------------------------------------------------------------
	// Start threads
	//---------------------------------------------------------------
	printf("Thread pool size : %d\n", m_nThreads);
	for(unsigned int i = 0; i < m_nThreads; ++i)
	{
		CZorgeThread* p = new CZorgeThread();
		m_vThreads.push_back(p);
		p->Start(ProcFunc, this);
	}

	m_pMonitor = new CZorgeThread();
	m_pMonitor->Start(ProcMonitorFunc, this);

	//---------------------------------------------------------------
	// Bound "listener".
	//---------------------------------------------------------------
	struct sockaddr_in SrvAddr;

	memset(&SrvAddr, 0, sizeof(SrvAddr));
	SrvAddr.sin_family = AF_INET; // Still with IPv4
	SrvAddr.sin_addr.s_addr = INADDR_ANY;
	SrvAddr.sin_port = htons(m_nPort);

	m_Socket = socket(AF_INET, SOCK_DGRAM, 0);
	int nRc = bind(m_Socket, (const struct sockaddr*)&SrvAddr, sizeof(SrvAddr));
	if(nRc == -1)
	{
		int n = errno;
		CZorgeString strErr;
		if(n == EADDRINUSE)
			strErr.Format("Server port %d in use by somebody else.", m_nPort);
		else
			strErr.Format("bind() port %d failed with errno = %d\n", m_nPort, n);
		throw CZorgeError(strErr);
	}
	printf("Listening port : %d\n", m_nPort);

	//---------------------------------------------------------------
	// Main loop
	//---------------------------------------------------------------
	CScopeGuard G(m_bRunning);

	struct sockaddr_in ClientAddr;
	memset(&ClientAddr, 0, sizeof(ClientAddr));
	socklen_t n = sizeof(ClientAddr);

	while(!m_bShutDown)
	{
		nRc = recvfrom(m_Socket, m_pRequestBuffer, m_nRequestBufferSize, 0, (sockaddr*)&ClientAddr, &n);
		if(m_bShutDown)
			break;
		if(nRc == -1)
		{
			int n = errno;
			printf("recvfrom() failed with errno = %d\n", n);
			return;
		}
		if(nRc == 0)
			continue;

		if(nRc < g_nUdpMessageSize && nRc != 13)
		{
			// Drop package with not expected length
			if(g_bReplyTraceOn)
				printf("Dropping package with size %d (%d)\n", nRc, g_nUdpMessageSize);
			continue;
		}

		try
		{
			m_MutexMonitorCall.Lock();
			++m_nMonitorCallsCount;
			m_MutexMonitorCall.Unlock();

			CUdpRequestItem* pReq = CreateRequest(nRc, ClientAddr);
			AddToQueue(pReq);
		}
		catch(ALL_ERRORS& e)
		{
			printf("Error : %s\n", e.what());
		}
	}
	puts("UDP server listener exit.");
}

// private
void CUdpServer::AddToQueue(CUdpRequestItem* pRequest)
{
	if(pthread_mutex_lock(&m_Mutex) != 0)
	{
		delete pRequest;
		throw CZorgeAssert("mutex lock failed. n != 0");
	}

	size_t nQueueSize = m_vQueue.size();
	if(nQueueSize == 0 && pRequest == 0)
	{
		if(pthread_mutex_unlock(&m_Mutex) != 0)
			throw CZorgeAssert("mutex lock failed. n != 0");
		return; // nothing to do.
	}

	if(nQueueSize > MAX_QUEUE_ITEMS_COUNT && pRequest)
	{
		if(pthread_mutex_unlock(&m_Mutex) != 0)
			throw CZorgeAssert("mutex unlock failed. n != 0");
		delete pRequest;
		puts("Queue is full");
		return;
	}

	if(pRequest != 0)
		m_vQueue.push_back(pRequest);
	else
		printf("AddToQueue. Forcing broadcast. size = %ld\n", m_vQueue.size());

	if(pthread_cond_broadcast(&m_Cond) != 0)
	{
		if(pthread_mutex_unlock(&m_Mutex) != 0)
			throw CZorgeAssert("mutex unlock failed. n != 0");

		throw CZorgeAssert("broadcast failed. Mutex still locked. n != 0");
	}

	if(pthread_mutex_unlock(&m_Mutex) != 0)
		throw CZorgeAssert("mutex unlock failed. n != 0");
}

static size_t g_nSockAddrSize = sizeof(struct sockaddr_in);
// private
CUdpRequestItem* CUdpServer::CreateRequest(int nDataSize, const struct sockaddr_in& Client)
{
	CUdpRequestItem* p = new CUdpRequestItem();
	memcpy((void*)&p->m_Client, (void*)&Client, g_nSockAddrSize);
	p->m_nDataSize = nDataSize;
	p->m_pData = new unsigned char[nDataSize];
	memcpy(p->m_pData, m_pRequestBuffer, nDataSize);

	// IPv4 only
	//struct sockaddr_in* pAddr_in = (struct sockaddr_in*)&Client;
	//char* szIp = inet_ntoa(pAddr_in->sin_addr);
	//strncpy(p->m_szCallerIp, szIp, IP_LEN - 1);
	//p->m_szCallerIp[IP_LEN - 1] = 0; // do it unconditionally, it's just faster

	return p;
}

#define CUR_THREAD_ID (unsigned int)pthread_self()
static void* ProcFunc(void* pParam)
{
	g_nRunningThreadCount++;
	CSetSignalHandler SignaHandler;

	CCoderAES* pCoder = new CCoderAES(COM_ENCODER);
	int nReplySocket = socket(AF_INET, SOCK_DGRAM, 0);
	TPerfBuffers Buffers;

	try
	{
		CUdpServer* pServer = (CUdpServer*)pParam;
		if(pServer == 0)
			throw CZorgeAssert("pServer == 0");
		if(pCoder == 0)
			throw CZorgeAssert("pCoder == 0");
		if(nReplySocket == -1)
			throw CZorgeAssert("ReplySocket == -1");

		CZorgeString strP("any"); // key will be replaced.
		pCoder->Init(strP);

		pthread_mutex_t& Mutex = pServer->m_Mutex;
		pthread_cond_t&	Cond = pServer->m_Cond;
		TQueue& Queue = pServer->m_vQueue;
		bool& bShutDown = pServer->m_bShutDown;

		printf("[%X] Thread started.\n", CUR_THREAD_ID);

		CUdpRequestItem* pItem = 0;
		while(1)
		{
			if(pthread_mutex_lock(&Mutex) != 0)
				throw CZorgeAssert("lock failed.");

			size_t n = Queue.size();
			if(n == 0)
			{
				// Wait for condition variable.
				if(pthread_cond_wait(&Cond, &Mutex) != 0)
					throw CZorgeAssert("Condition wait failed.");
			}

			if(bShutDown)
			{
				if(pthread_mutex_unlock(&Mutex) != 0)
					throw CZorgeAssert("Unlock failed.");
				break;
			}

			if(pthread_mutex_unlock(&Mutex) != 0)
				throw CZorgeAssert("Unlock failed.");

			while(1)
			{
				pItem = 0;
				if(pthread_mutex_lock(&Mutex) != 0)
					throw CZorgeAssert("lock failed.");

				if(Queue.size() != 0)
				{
					TQueue::iterator it = Queue.begin();
					pItem = *it;
					Queue.erase(it);
				}
				else
				{
					if(pthread_mutex_unlock(&Mutex) != 0)
						throw CZorgeAssert("Unlock failed.");
					break;
				}

				if(pthread_mutex_unlock(&Mutex) != 0)
					throw CZorgeAssert("Unlock failed.");

				if(!pItem)
					continue;

				// Process request
				pServer->ProcessRequest(pItem, Buffers, nReplySocket, pCoder);
			}
		}
	}
	catch(ALL_ERRORS& e)
	{
		printf("[%X] Thread error. %s\n", CUR_THREAD_ID, e.what());
	}

	if(pCoder)
		delete pCoder;
	close(nReplySocket);

	printf("[%X] Thread exit.\n", CUR_THREAD_ID);
	g_nRunningThreadCount--;
	return 0;
}

unsigned int CUdpServer::GetCallCount()
{
	m_MutexMonitorCall.Lock();
	unsigned int n = m_nMonitorCallsCount;
	m_nMonitorCallsCount = 0;
	m_MutexMonitorCall.Unlock();
	return n;
}

void CUdpServer::ShutDown()
{
	puts("UDP server got signal to shut down.");
	m_bShutDown = true;
	close(m_Socket);
	// broadcasting to exit threads from the pool
	if(pthread_cond_broadcast(&m_Cond) != 0)
		puts("broadcast failed.");
}

static void* ProcMonitorFunc(void* pParam)
{
	g_bMonitorRunning = true;
	CSetSignalHandler SignaHandler;

	try
	{
		CUdpServer* pServer = (CUdpServer*)pParam;
		if(pServer == 0)
			throw CZorgeAssert("pServer == 0");

		unsigned int m_nMonitorTimeoutSec = pServer->m_nMonTimeOutSec;

		printf("[%X] Monitor thread started.\n", CUR_THREAD_ID);
		unsigned int nTimeoutSec = m_nMonitorTimeoutSec;
		printf("Monitor timeout : %d sec\n", nTimeoutSec);
		unsigned int nTotalIter = nTimeoutSec * 2;
		if(nTotalIter == 0)
			throw CZorgeAssert("nTotalIter == 0");

		unsigned int i = 0, nCount = 0;
		float nCallsPerSecond = 0.0;
		CZorgeString strLine(128);

		bool& bShutDown = pServer->m_bShutDown;

		size_t nQueueSize = 0;
		while(bShutDown == false)
		{
			for(i = 0; i < nTotalIter && bShutDown == false; ++i)
				ZorgeSleep(500);

			if(bShutDown)
				break;

			if(pthread_mutex_lock(&pServer->m_Mutex) != 0)
				throw CZorgeAssert("Lock failed.");
			nQueueSize = pServer->m_vQueue.size();
			if(pthread_mutex_unlock(&pServer->m_Mutex) != 0)
				throw CZorgeAssert("Unlock failed.");

			// Do monitoring job
			TimeNowToString(strLine);
			nCount = pServer->GetCallCount();
			nCallsPerSecond = (float)nCount / (float)nTimeoutSec;
			printf("%s %.1f c/sec, %ld KB, queue %ld\n", (const char*)strLine, nCallsPerSecond, GetResMemUsage_KB(), nQueueSize);
		}
	}
	catch(ALL_ERRORS& e)
	{
		printf("[%X] Monitor thread error. %s\n", CUR_THREAD_ID, e.what());
	}

	printf("[%X] Monitor thread exit.\n", CUR_THREAD_ID);
	g_bMonitorRunning = false;
	return 0;
}

// private
size_t CUdpServer::GetData(const CUdpRequestItem* pRequest, CUdpMessage& Msg, CCoderAES* pCoder, TPerfBuffers& Buffers, CZorgeString& strOut, TTokens* pvTokens)
{
	CUdpMessageEncr* pEncr = (CUdpMessageEncr*)pRequest->m_pData;
	if(!pEncr->m_chEncr)
	{
		// Message not encrypted. Only CONNECT allowed. Caller will assert that.
		CUdpMessage* pNotEncr = (CUdpMessage*)pRequest->m_pData;
		Msg.m_chEncr = pNotEncr->m_chEncr;
		Msg.m_chKeyId = pNotEncr->m_chKeyId;
		Msg.m_nCmdId = pNotEncr->m_nCmdId;
		Msg.m_nSeqNum = pNotEncr->m_nSeqNum;
		Msg.m_chReplyStatus = pNotEncr->m_chReplyStatus;
		memcpy(Msg.m_szClientId, pNotEncr->m_szClientId, CLIENT_ID_LEN);
		Msg.m_nDataLen = pNotEncr->m_nDataLen;

		if(Msg.m_nDataLen == 0)
			strOut.Empty();
		else
		{
			++pNotEncr;
			strOut.SetData((const char*)pNotEncr, Msg.m_nDataLen);
		}
		return 0;
	}

	// Message is encrypted.
	Msg.m_chEncr = 1; 					// Comes not encrypted.
	Msg.m_chKeyId = pEncr->m_chKeyId; 	// Comes not encrypted.
	size_t nEncrLen = pEncr->m_nEncrLen;// Comes not encrypted.

	const CZorgeString* strKey = g_Keys.GetKey(Msg.m_chKeyId);

	CUdpMessageEncr* pEncrData = pEncr;
	++pEncrData;

	CMemBuffer& BufIn = Buffers.m_EncrIn;
	CMemBuffer& BufOut = Buffers.m_EncrOut;
	BufIn.Init((unsigned char*)pEncrData, nEncrLen);
	pCoder->Init(*strKey);
	pCoder->Decrypt(BufIn, BufOut);

	// 2 bytes command id
	// 4 bytes sequence number
	// 1 byte reply status
	// 32 bytes client id
	// 4 bytes data length
	unsigned char* pDecrBuf = BufOut.Get();
	Msg.m_nCmdId = *((short*)pDecrBuf);
	pDecrBuf += sizeof(short);

	Msg.m_nSeqNum = *((unsigned int*)pDecrBuf);
	pDecrBuf += sizeof(int);

	Msg.m_chReplyStatus = *pDecrBuf;
	pDecrBuf += sizeof(unsigned char);
	memcpy(Msg.m_szClientId, (const char*)pDecrBuf, CLIENT_ID_LEN);

	pDecrBuf += CLIENT_ID_LEN;

	Msg.m_nDataLen = *((unsigned int*)pDecrBuf);
	pDecrBuf += sizeof(int);

//	printf("nCmdId = %d %s, %d %s\n", nCmdId, szClientId, nDataLen, szData);
	if(pvTokens != 0)
	{
		unsigned char* szData = pDecrBuf;
		strOut.SetData((const char*)szData, Msg.m_nDataLen);

		TTokens& v = *pvTokens;
		strOut.Split('\n', v);
		return v.size();
	}
	return 0; // to make compiler happy.
}

static const char* msgIdToString(unsigned short n)
{
	switch(n)
	{
		case CMD_HANDSHAKE:		return "CMD_HANDSHAKE";
		case CMD_CONNECT: 		return "CMD_CONNECT";

		case CMD_GET_REPLY: 	return "CMD_GET_REPLY";
		case CMD_REPO_LOGIN: 	return "CMD_REPO_LOGIN";
		case CMD_REPO_STATUS: 	return "CMD_REPO_STATUS";
		case CMD_REPO_INIT: 	return "CMD_REPO_INIT";

		case CMD_NUM_ADD: 		return "CMD_NUM_ADD";
		case CMD_NUM_FIND: 		return "CMD_NUM_FIND";
		case CMD_NUM_DEL: 		return "CMD_NUM_DEL";
		case CMD_NUM_OPEN: 		return "CMD_NUM_OPEN";

		case CMD_TOKEN_GET:		return "CMD_TOKEN_GET";
		case CMD_TOKEN_NEW:		return "CMD_TOKEN_NEW";
		case CMD_TOKEN_DELETE:	return "CMD_TOKEN_DELETE";

		case CMD_MSG_COUNT: 	return "CMD_MSG_COUNT";
		case CMD_MSG_SEND: 		return "CMD_MSG_SEND";
		case CMD_MSG_GET_IDS: 	return "CMD_MSG_GET_IDS";
		case CMD_MSG_READ: 		return "CMD_MSG_READ";
		case CMD_MSG_DELETE: 	return "CMD_MSG_DELETE";
	}
	return "unknown command.";
}

// private
void CUdpServer::GetReply(CUdpMessage& Msg, CZorgeString& strReply)
{
	strReply.Empty();

	bool bError = false;
	CacheItemStatus Status = m_OpenConsCache.Find(Msg.m_szClientId, Msg.m_nSeqNum, bError, strReply);
	switch(Status)
	{
		case InProcessing: 	Msg.m_chReplyStatus = REPLY_STATUS_IN_PROCESS; break;
		case Done: 			Msg.m_chReplyStatus = REPLY_STATUS_DONE; break;
		case NotFound: 		Msg.m_chReplyStatus = REPLY_STATUS_NOT_FOUND; break;
		default:
			printf("Not expected value ! %d\n", Status);
	}

	const char* szStatus = 0;
	switch(Status)
	{
		case 0 : szStatus = "In process"; break;
		case 1 : szStatus = "Done"; break;
		case 2 : szStatus = "Not found"; break;
		default: szStatus = "Unknown";
	}

	char szTmp[CLIENT_ID_LEN + 1];
	memcpy(szTmp, Msg.m_szClientId, CLIENT_ID_LEN);
	szTmp[CLIENT_ID_LEN] = 0;
	printf("GetReply(%X, %s) status=%s, error=%d\n", Msg.m_nSeqNum, szTmp, szStatus, bError);
	if(bError)
		throw CZorgeError(strReply);
}

// private
void CUdpServer::ProcessRequest(CUdpRequestItem* pRequest, TPerfBuffers& Buffers, int nReplySocket, CCoderAES* pCoder)
{
	if(pRequest == 0)
		throw CZorgeAssert("pRequest == 0");

	CUdpMessage Msg;
	memset(&Msg, 0, sizeof(Msg));

	//-------------------------------------------------------
	// Do processing
	//-------------------------------------------------------
	unsigned char chError = 0;
	unsigned short nCmd = 0;
	bool bNotEncrCmd = false;
	CZorgeString strInput, strReply;
	try
	{
		TTokens v;
		size_t n = GetData(pRequest, Msg, pCoder, Buffers, strInput, &v);

		nCmd = Msg.m_nCmdId;
		bool bValidCmd = IS_VALID_COMMAND(nCmd);
		if(!bValidCmd)
		{
			delete pRequest;
			return; // Black holing packet.
		}

		bNotEncrCmd = nCmd == CMD_GET_REPLY || nCmd == CMD_HANDSHAKE || nCmd == CMD_CONNECT;
		if(Msg.m_chEncr == 0 && bNotEncrCmd == false)
			throw CZorgeError("Not encrypted call.");

		Msg.m_chReplyStatus = REPLY_STATUS_IN_PROCESS;

		if(nCmd != CMD_GET_REPLY && bNotEncrCmd == false)
			m_OpenConsCache.Add(Msg.m_szClientId, Msg.m_nSeqNum);

		if(0)
		{
			unsigned long nThreadId = (long)pthread_self();
			printf("[%lx] %s\n", nThreadId, msgIdToString(nCmd));
		}

		if(nCmd == CMD_HANDSHAKE)
		{
			// Client sent version, expecting back pky
			if(strInput != "1")
			{
				strReply.Format("Not supported client version [%s]", (const char*)strInput);
				throw CZorgeError(strReply);
			}

			strReply = m_pCoderRSA->GetPublicKey();
		}
		else if(nCmd == CMD_CONNECT)
		{
			// Client sent encrypted password, expecting session token back
			CZorgeString strClientKey;
			CMemBuffer BufEncr, BufDec;
			if(BufEncr.InitFromHexChars(strInput) != 0)
				throw CZorgeError("Invalid input");

			m_pCoderRSA->Decrypt(BufEncr, BufDec);
			BufDec.ToString(strClientKey);

			CZorgeString strSessionKey, strOut;
			unsigned char ch = g_Keys.GetNextKey(strSessionKey);
			strOut.Format("%02X ", ch);
			strOut += strSessionKey;

			// Encrypt session key with client's key
			m_pCoderAES->Init(strClientKey);
			m_pCoderAES->EncryptToHexChars(strOut, strReply);
		}
		else if(nCmd == CMD_GET_REPLY)
		{
			GetReply(Msg, strReply);
		}
		else if(nCmd == CMD_NUM_ADD)
		{
			// input: number pin
			// output: new_number
			if(n != 2)
				throw CZorgeError("Invalid parameter count for add number.");
			m_pStorage->AddNumber(v[0], v[1], strReply);
		}
		else if(nCmd == CMD_NUM_DEL)
		{
			// input: number pin
			// output:
			if(n != 2)
				throw CZorgeError("Invalid parameter count for delete number.");
			m_pStorage->DeleteNumber(v[0], v[1]);
		}
		else if(nCmd == CMD_MSG_GET_IDS)
		{
			// input: number pin 1/0
			// output: array of message ids
			if(n != 3)
				throw CZorgeError("Invalid parameter count for get messages ids.");

			CNumDev OpenNum = {v[0], v[1]};
			const char* szType = (const char*)v[2];
			if(szType == 0)
				throw CZorgeError("Invalid parameter value for NewOnly.");

			int nType = MSG_NEW;
			if(*szType == '1')
				nType = MSG_NEW;
			else if(*szType == '0')
				nType = MSG_ALL;
			else
				throw CZorgeError("Invalid parameter value for NewOnly.");

			TIds vIds;
			m_pStorage->GetIds(OpenNum, nType, vIds);
			for(TIds::iterator it = vIds.begin(); it != vIds.end(); ++it)
			{
				strReply += **it;
				strReply += "\n";
				delete *it;
			}
		}
		else if(nCmd == CMD_NUM_OPEN)
		{
			// input: number pin
			// output:
			if(n != 2)
				throw CZorgeError("Invalid parameter count for open.");
			m_pStorage->OpenNumber(v[0], v[1]);
		}
		else if (nCmd == CMD_NUM_CHANGE_PIN)
		{
			// input: number pin newPin
			// output:
			if(n != 3)
				throw CZorgeError("Invalid parameter count for change pin.");

			CNumDev Num = {v[0], v[1]};
			m_pStorage->ChangePin(Num, v[2]);
		}
		else if(nCmd == CMD_MSG_SEND)
		{
			// input: number pin toNumber message_to_send
			// output: message id
			if(n < 3) // this will solve issue with message having '\n'-s
				throw CZorgeError("Invalid parameter count for send.");

			CNumDev OpenNum = {v[0], v[1]};

			int nPos = v[0].GetLength() + v[1].GetLength() + v[2].GetLength() + 3;
			CZorgeString strMsg;
			const CZorgeString& strToNum = v[2];

			const char* szMsg = (const char*)strInput;
			szMsg += nPos;
			strMsg = szMsg;

			m_pStorage->SendMsg(OpenNum, strToNum, strMsg, strReply);
		}
		else if(nCmd == CMD_MSG_READ)
		{
			// input: number pin msgId
			// output: text
			if(n != 3)
				throw CZorgeError("Invalid parameter count for read.");

			CNumDev OpenNum = {v[0], v[1]};
			CMessage* pMsg = m_pStorage->ReadMessage(OpenNum, v[2]);
			if(pMsg == 0)
				throw CZorgeError("Message not found.");

			static CZorgeString strSeparator = "]@@]";
			CZorgeString strTmp;
			strReply = MessageStatusToString(pMsg->m_Record.m_Data.m_nStatus); strReply += strSeparator;
			strTmp.Format("%ld", pMsg->m_Record.m_Data.m_nTimeCreated);
			strReply += strTmp; strReply += strSeparator;
			strReply += pMsg->m_Record.m_Data.m_szFromNum; strReply += strSeparator;
			strReply += pMsg->m_strText;
			delete pMsg;
		}
		else if(nCmd == CMD_MSG_DELETE)
		{
			// input: number pin msgId
			// output:
			if(n != 3)
				throw CZorgeError("Invalid parameter count for delete.");

			CNumDev OpenNum = {v[0], v[1]};
			m_pStorage->DeleteMessage(OpenNum, v[2]);
			strReply.EmptyData();
		}
		else if(nCmd == CMD_MSG_COUNT)
		{
			// input: number pin
			// output: newMsgCount
			if(n != 2)
				throw CZorgeError("Invalid parameter count for check.");

			CNumDev OpenNum = {v[0], v[1]};
			unsigned int nTotal(0), nNew(0);
			m_pStorage->GetMsgCount(OpenNum, nTotal, nNew);
			strReply.Format("%d %d", nTotal, nNew);
		}
		else if(nCmd == CMD_DIAGNOSTIC)
		{
			// input:
			// output: lines
			m_OpenConsCache.Diagnostics(strReply);
			m_pStorage->Diagnostics(strReply);
			DiagCUdpRequestItem(strReply);
		}
		else if(nCmd == CMD_TOKEN_GET)
		{
			// input: number pin
			// output: token
			if(n != 2)
				throw CZorgeError("Invalid parameter count for get token.");

			CNumDev OpenNum = {v[0], v[1]};
			strReply = m_pStorage->GetNumToken(OpenNum);
		}
		else if(nCmd == CMD_TOKEN_NEW)
		{
			// input: number pin
			// output: token
			if(n != 2)
				throw CZorgeError("Invalid parameter count for new token.");

			CNumDev OpenNum = {v[0], v[1]};
			strReply = m_pStorage->NewNumToken(OpenNum);
		}
		else if(nCmd == CMD_TOKEN_DELETE)
		{
			// input: number pin
			// output:
			if(n != 2)
				throw CZorgeError("Invalid parameter count for delete token.");

			CNumDev OpenNum = {v[0], v[1]};
			m_pStorage->DeleteNumToken(OpenNum);
			strReply.Empty();
		}
		else if(nCmd == CMD_TOKEN_CHECK)
		{
			// input: number token
			// output: true/false
			if(n != 2)
				throw CZorgeError("Invalid parameter count for check token.");

			CNumDev Num = {v[0], v[1]};
			bool bValid = m_pStorage->CheckNumToken(Num);
			strReply = bValid ? "true" : "false";
		}
		else
		{
			strReply.Format("Unknown command id %d", Msg.m_nCmdId);
			throw CZorgeError(strReply);
		}
	}
	catch(ALL_ERRORS& e)
	{
		chError = 1;
		strReply = e.what();
		printf("Processing error : %s\n", e.what());
	}

	if(nCmd != CMD_GET_REPLY && bNotEncrCmd == false)
		m_OpenConsCache.Update(Msg.m_szClientId, Msg.m_nSeqNum, chError, strReply);

	//-------------------------------------------------------
	// Send reply
	//-------------------------------------------------------
	bool bEncr = Msg.m_chEncr == 1;
	unsigned int nDataLen = strReply.GetLength();
	unsigned char* szData = (unsigned char*)(const char*)strReply;
	CMemBuffer& BufIn = Buffers.m_EncrIn;
	CMemBuffer& BufOut = Buffers.m_EncrOut;

	if(bEncr)
	{
		BufIn.Init(szData, nDataLen);
		pCoder->Encrypt(BufIn, BufOut);
		nDataLen = BufOut.GetSizeData();
		szData = BufOut.Get();
	}

	Msg.m_nDataLen = nDataLen;
	size_t nTotalLen = g_nUdpMessageSize + Msg.m_nDataLen + 1; // 1 for error byte

	CMemBuffer& Buf = Buffers.m_Reply;
	unsigned char* p = Buf.Allocate(nTotalLen);
	memcpy(p, &Msg, g_nUdpMessageSize);
	p[g_nUdpMessageSize] = chError;
	memcpy(p + g_nUdpMessageSize + 1, (void*)szData, Msg.m_nDataLen);

	socklen_t n = g_nSockAddrSize;
	int nRc = sendto(nReplySocket, p, nTotalLen, 0, (sockaddr*)&pRequest->m_Client, n);
	if(nRc == -1)
	{
		int n = errno;
		printf("sendto() failed with errno = %d\n", n);
	}

	delete pRequest;
}
