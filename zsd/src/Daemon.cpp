#include <stdlib.h>
#include "Daemon.h"
#include "ZorgeError.h"
#include "SysUtils.h"

//------------------------------------------------------------
//
//------------------------------------------------------------
CDaemon::CDaemon()
{
	m_pUdpServer = 0;
	m_pStorage = 0;
	m_bShutDown = false;
}

//------------------------------------------------------------
//
//------------------------------------------------------------
CDaemon::~CDaemon()
{
	if(m_pStorage)
		delete m_pStorage;

	if(m_pUdpServer)
		delete m_pUdpServer;
}

static const CZorgeString& InputLine(CZorgeString& strLine)
{
	char szLine[2048];
	fflush(stdout);
	if(fgets(szLine, sizeof(szLine), stdin) == 0)
		szLine[0] = 0;
	strLine = szLine;
	strLine.Trim();
	return strLine;
}

// private
void CDaemon::DealWithUser(const CZorgeString& strRoot)
{
	bool bRootExist = IsFileExist((const char*)strRoot) == FOLDER_EXIST;
	bool bConfigExist = false;
	if(bRootExist)
	{
		CZorgeString strConfig(strRoot);
		if(!strConfig.IsLastChar('/'))
			strConfig += "/";
		strConfig += "Config";

		bConfigExist = IsFileExist((const char*)strConfig) == FILE_EXIST;
	}

	if(bConfigExist)
		return; // Ready to login

	CZorgeString strLine;
	puts("------------------------------------------------------");
	printf("Storage not initialized at '%s'\n", (const char*)strRoot);
	printf("Do you want to initialize ? [Y]");
	InputLine(strLine);
	bool bYes = strLine.IsEmpty() || strLine == "Y" || strLine == "y";
	if(!bYes)
		return;

	printf("\nEnter new storage password : ");
	InputLine(strLine);
	if(strLine.IsEmpty())
		throw CZorgeError("Password can not be empty");
	CZorgeString strPsw(strLine);

	for(int i = 0; i < 3; ++i)
	{
		printf("Confirm new storage password : ");
		InputLine(strLine);
		if(strLine == strPsw)
			break;
		puts("Password not match. Try again.\n\n");
	}

	if(strLine != strPsw)
		throw CZorgeError("You did fail to confirm password.");

	// Create new storage folder.
	MkPath(strRoot);

	// Do initialize.
	printf("\nInitializing '%s' with new password.\n", (const char*)strRoot);
	m_pStorage->Init(strPsw);

	CStorage::Status nStatus = m_pStorage->GetStatus();
	if(nStatus == CStorage::Status::Open)
		puts("Storage initialized.\n");
	else
		throw CZorgeError("Failed to initialize.");
}

//------------------------------------------------------------
//
//------------------------------------------------------------
void CDaemon::Run(const char* szStorageRoot)
{
	CZorgeString strRoot(szStorageRoot);

	m_pStorage = new CStorage();
	m_pStorage->SetRoot(strRoot);
	printf("\nStorage root : %s\n", (const char*)strRoot);
	printf("Status : %s\n", m_pStorage->GetStatusStr());

	CStorage::Status nStatus = m_pStorage->GetStatus();
	if(nStatus != CStorage::Status::ReadyToOpen)
	{
		DealWithUser(strRoot);
		nStatus = m_pStorage->GetStatus();
	}

	if(nStatus != CStorage::Status::ReadyToOpen && nStatus != CStorage::Status::Open)
		throw CZorgeError("Invalid storage status.");

	int nTries = 0;
	while(nStatus != CStorage::Status::Open)
	{
		try
		{
			CZorgeString strStorageKey;
			printf("\nEnter storage password : ");
			EnterPassword(strStorageKey);

			if(m_bShutDown)
				return; // user press Ctrl-C during password entering.

			m_pStorage->Login(strStorageKey);
			strStorageKey.EmptyData(); // I am not storing it in the core dump
			if(m_pStorage->GetStatus() != CStorage::Status::Open)
				throw CZorgeError("Failed to open storage with key");
			puts("Storage is open.");
			break;
		}
		catch(CZorgeError& e) {
			printf("\n%s\n", (const char*)e.what());
			if(++nTries == 3)
				throw CZorgeError("Invalid password.");
		}
	}

	CZorgeString strTmp;
	CConfig& Config = GetConfig();

	if(!Config.GetValue(CONFIG_LISTENER_PORT, strTmp))
		throw CZorgeError("Listener.Port not found in the config.");
	int nPort = atoi((const char*)strTmp);
	if(nPort < 1)
		throw CZorgeError("Listener.Port has invalid value in the config.");

	if(!Config.GetValue(CONFIG_THREAD_POOL_SIZE, strTmp))
		throw CZorgeError("Listener.Port not found in the config.");
	unsigned int nThreads = atoi((const char*)strTmp);
	if(nThreads < 1)
		throw CZorgeError("Thread.Pool.Size has invalid value in the config.");

	if(!Config.GetValue(CONFIG_THREAD_POOL_MONITOR_TIMEOUT, strTmp))
		throw CZorgeError("Thread.Pool.Monitor.Timeout not found in the config.");
	unsigned int nTimeout = atoi((const char*)strTmp);
	if(nThreads < 1)
		throw CZorgeError("Thread.Pool.Monitor.Timeout has invalid value in the config.");

	m_pUdpServer = new CUdpServer(nPort, nThreads);
	m_pUdpServer->SetStorage(m_pStorage);
	m_pUdpServer->SetMonitorTimeout(nTimeout);
	m_pUdpServer->Run();
}

//------------------------------------------------------------
//
//------------------------------------------------------------
void CDaemon::ShutDown()
{
	m_bShutDown = true;

	if(m_pUdpServer)
		m_pUdpServer->ShutDown();
}
