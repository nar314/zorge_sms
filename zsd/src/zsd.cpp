#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "ZorgeError.h"
#include "Daemon.h"


CDaemon g_Daemon;

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
void CtrlC(int nSignal)
{
	if(nSignal == SIGINT)
	{
		puts("\nStopping demon.");
		g_Daemon.ShutDown();
	}
	else
		puts("\nNot expected signal.");
}

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
int main(int argc, char** argv)
{
	CSetSignalHandler SignaHandler;
	signal(SIGINT, CtrlC);

	CZorgeString strStorage;
	char* szRoot = getenv(ZSMS_STORAGE_PATH);
	if(szRoot)
	{
		strStorage = szRoot;
		if(!strStorage.IsLastChar('/'))
			strStorage += "/";
	}

	puts("Starting Zorge SMS deamon.");
	if(argc == 2)
	{
		strStorage = argv[1];
		if(strcmp(argv[1], "/?") == 0)
		{
			puts("Usage : ./zsd <path to storage folder>");
			puts("\nWhen started without parameter, folder can be set by ZSMS_STORAGE_PATH.");
			puts("export ZSMS_STORAGE_PATH=/tmp/folder1/folder2\n\n\n");
			return 1;
		}
	}

	if(argc > 2)
	{
		puts("Invalid command line parameter. Expected: ./zsd [/path/to/storage/folder]");
		return 2; // Invalid command line parameter
	}

	if(strStorage.IsEmpty())
	{
		char szCurFolder[2048];
		strStorage = getcwd(szCurFolder, sizeof(szCurFolder));
		if(!strStorage.IsEmpty() && !strStorage.IsLastChar('/'))
			strStorage += "/";
		strStorage += "zsms_storage/";
	}

	if(strStorage.IsEmpty())
	{
		puts("\nEnvironmental variable ZSMS_STORAGE_PATH not defined and command line parameter is missing.");
		puts("To provide path to storage use:");
		puts("export ZSMS_STORAGE_PATH=/tmp/folder1/folder2 or");
		puts("./zsd /tmp/folder1/folder2\n\n");
		return 2; // Invalid command line parameter
	}

	const char* szStorageRoot = (const char*)strStorage;
	try
	{
		g_Daemon.Run(szStorageRoot);
	}
	catch(ALL_ERRORS& e)
	{
		printf("Error : %s\n", e.what());
		g_Daemon.ShutDown();
		return 1;
	}
	return 0;
}

// FIX ME. Add watch dog for daemon.
// FIX ME. Add ability to check firewall settings.
// sudo iptables -A INPUT -p udp -m udp --dport 7530 -j ACCEPT https://www.geeksforgeeks.org/iptables-command-in-linux-with-examples/
// When server rebooted, client will get "Error : Failed to decrypt reply. Given final block not properly padded. Such issues can arise if a bad key is used during decryption."
