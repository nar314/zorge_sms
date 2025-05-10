// Daemon.h

#ifndef C_DAEMON_H_
#define C_DAEMON_H_

#include "UdpServer.h"
#include "Storage.h"

//------------------------------------------------------------
//
//------------------------------------------------------------
class CDaemon
{
	CUdpServer* m_pUdpServer;
	CStorage* 	m_pStorage;
	bool		m_bShutDown;

	void		DealWithUser(const CZorgeString& strRoot);

public:
	CDaemon();
	~CDaemon();

	void Run(const char* szStorageRoot);
	void ShutDown();
};

#endif
