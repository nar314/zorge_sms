// UdpServer.h

#ifndef C_UDPSERVER_H_
#define C_UDPSERVER_H_

#include <arpa/inet.h>

#include <vector>
using namespace std;

#include "ZorgeThread.h"
#include "Storage.h"
#include "OpenConsCache.h"
#include "CoderAES.h"
#include "CoderRSA.h"

//#define IP_LEN 16 // 3 * 4 + 3 + 1 <- include null terminator

struct CUdpRequestItem
{
	struct sockaddr_in	m_Client;
	unsigned int		m_nDataSize;
	unsigned char*		m_pData;	// pointer to CUdpMessage
	//char				m_szCallerIp[IP_LEN];

	CUdpRequestItem();
	CUdpRequestItem(const CUdpRequestItem&) = delete;
	~CUdpRequestItem();
	void Print() const;
};

#define CLIENT_ID_LEN 32 // no 0x00

#define  REPLY_STATUS_IN_PROCESS 	'0'
#define  REPLY_STATUS_DONE 			'1'
#define  REPLY_STATUS_NOT_FOUND 	'2'

#pragma pack(push, 1)
struct CUdpMessageEncr
{
	unsigned char 	m_chEncr;
	unsigned char 	m_chKeyId;
	unsigned int	m_nEncrLen;
};

// Same structure for request and respond
struct CUdpMessage
{
	unsigned char	m_chEncr;			// is encrypted
	unsigned char	m_chKeyId;			// encryption key id
	unsigned short	m_nCmdId;			// command to execute
	unsigned int	m_nSeqNum;			// sequence number
	unsigned char	m_chReplyStatus;	// reply status
	char			m_szClientId[CLIENT_ID_LEN]; // client id as modified GUID. Not null terminated string !
	unsigned int	m_nDataLen;			// command input data length
};
#pragma pack(pop)

typedef vector<CZorgeThread*> TThreads;
typedef vector<CUdpRequestItem*> TQueue;

// Buffers to improve performance.
typedef struct
{
	CMemBuffer m_EncrIn;
	CMemBuffer m_EncrOut;
	CMemBuffer m_Reply;
} TPerfBuffers;

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
class CUdpServer
{
	int				m_nPort;
	unsigned char*	m_pRequestBuffer;
	size_t			m_nRequestBufferSize;
	int				m_Socket;

	bool			m_bRunning;

	unsigned int	m_nThreads;
	TThreads		m_vThreads;
	CZorgeThread*	m_pMonitor;
	unsigned int	m_nMonitorCallsCount;
	CZorgeMutex		m_MutexMonitorCall;
	CCoderRSA*		m_pCoderRSA;
	CCoderAES*		m_pCoderAES;

	CStorage*		m_pStorage;
	COpenConsCache	m_OpenConsCache;

	CUdpRequestItem* CreateRequest(int nDataSize, const struct sockaddr_in& Client);
	size_t 			GetData(const CUdpRequestItem* pRequest, CUdpMessage& Msg, CCoderAES* pCoder, TPerfBuffers& Buffers, CZorgeString& strOut, TTokens* pvTokens);
	void 			GetReply(CUdpMessage& Msg, CZorgeString& strReply);

public:
	pthread_mutex_t	m_Mutex;
	pthread_cond_t	m_Cond;
	TQueue			m_vQueue;
	bool			m_bShutDown;
	unsigned int	m_nMonTimeOutSec;

	CUdpServer(const CUdpServer&) = delete;
	CUdpServer& operator=(const CUdpServer&) = delete;

	CUdpServer(int nPort, unsigned int nThreads);
	~CUdpServer();

	void			Run();
	void			ShutDown();
	void			SetMonitorTimeout(unsigned int nTimeOut);
	void			SetStorage(CStorage* p);

	// Methods called from statics
	void			ProcessRequest(CUdpRequestItem* pRequest, TPerfBuffers& Bufs, int nReplySocket, CCoderAES* pCoder);
	void			AddToQueue(CUdpRequestItem* pRequest);
	unsigned int 	GetCallCount();
};

#endif
