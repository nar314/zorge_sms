#include "ZorgeError.h"
#include "Storage.h"
#include "SysUtils.h"

#include <string>
#include <iostream>

static CStorage* g_pStorage = 0;
static CNumDev g_OpenNumDev;

static void GetLineTerminal(CZorgeString& strOut)
{
	strOut.EmptyData();

	string s;
	getline(cin, s);
	strOut = s.c_str();
}

const char* TimeTtoString(time_t nTime, CZorgeString& strOut)
{
	static const char* g_szMon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	struct tm* pTime = localtime(&nTime);

	const char* szMonth = pTime->tm_mon > 11 ? "???" : g_szMon[pTime->tm_mon];
	char szTime[32];
	snprintf(szTime, 32, "%02d %s %04d %02d:%02d:%02d",
			pTime->tm_mday, szMonth, pTime->tm_year + 1900,
			pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
	strOut = szTime;
	return (const char*)strOut;
}

static const char* FormatMsgIdLine(const CZorgeString& strLine, CZorgeString& strOut)
{
	// N 1732477978 101 7F0608501A2B482C933211AEBA669FB7
	// N {24 Nov 2024 09:52:58} 101 7F0608501A2B482C933211AEBA669FB7
	TTokens vTokens;
	strLine.Split(' ', vTokens);
	if(vTokens.size() != 4)
	{
		strOut = strLine;
		return (const char*)strOut;
	}

	CZorgeString strLocal;
	time_t nUtc = atol(vTokens[1]);
	time_t nLocal = UtcToLocal(nUtc);
	TimeTtoString(nLocal, strLocal);

	strOut = vTokens[0]; strOut += " {";
	strOut += strLocal; strOut += "} ";
	strOut += vTokens[2]; strOut += " ";
	strOut += vTokens[3];
	return (const char*)strOut;
}

void PrintHelpToken()
{
	puts("token get");
	puts("token new");
	puts("token delete");
	puts("token check <token>");
}

void PrintHelpMessage()
{
	puts("msg send <toNumber> <messageToSend>");
	puts("msg check");
	puts("msg getIds <new/all>");
	puts("msg read <id>");
	puts("msg delete <id>");
	puts("msg sendToken <from num> <to num> <from num token> <msg>");
}

void PrintHelpNumber()
{
	puts("num add <number> <pin>");
	puts("num find <number>");
	puts("num del <number> <pin>");
	puts("num open <number> <pin>");
	puts("num close");
}

void PrintHelpRepo()
{
	puts("repo status");
	puts("repo login <key>");
	puts("repo init <key>");
}

void PrintHelp()
{
	PrintHelpRepo();
	puts("");
	PrintHelpNumber();
	puts("");
	PrintHelpMessage();
	puts("");
	PrintHelpToken();
	puts("");

	puts("exit");
	puts("help");
	puts("diag");
	puts("?");
}

bool ProcessMessageCmd(const TTokens& vTokens)
{
	size_t n = vTokens.size();
	if(n <= 1)
	{
		puts("Invalid parameters count.\n");
		PrintHelpMessage();
		return false;
	}
	const CZorgeString& str = vTokens.at(1);

	if(g_OpenNumDev.m_strNum.IsEmpty() && str != "sendToken")
	{
		puts("Number is not open.");
		return false;
	}

	try
	{
		if(str == "check")
		{
			// $msg check
			unsigned int nTotal(0), nNew(0);
			g_pStorage->GetMsgCount(g_OpenNumDev, nTotal, nNew);
			printf("Total messages %d, new messages %d\n", nTotal, nNew);
			return true;
		}

		if(str == "send")
		{
			// $msg send 102 Hello
			if(n != 4)
			{
				puts("Invalid parameters count.\n");
				PrintHelpMessage();
				return false;
			}
			const CZorgeString& strToNum = vTokens.at(2);
			CZorgeString strMsg = vTokens.at(3);
			if(strMsg == "\"\"")
				strMsg.EmptyData();
			CZorgeString strMsgId;
			g_pStorage->SendMsg(g_OpenNumDev, strToNum, strMsg, strMsgId);
			printf("Message id = %s\n", (const char*)strMsgId);
			return true;
		}

		if(str == "getIds")
		{
			// $msg getIds new
			if(n != 3)
			{
				puts("Invalid parameters count.\n");
				PrintHelpMessage();
				return false;
			}

			const CZorgeString& strType = vTokens.at(2);
			int nType = -1;
			if(strType == "new")
				nType = MSG_NEW;
			else if(strType == "all")
				nType = MSG_ALL;
			else
			{
				puts("Invalid message type. Expected new or all");
				return false;
			}

			TIds vIds;
			CZorgeString strTmp;
			g_pStorage->GetIds(g_OpenNumDev, nType, vIds);
			size_t nTotal = vIds.size();
			TIds::iterator it = vIds.begin();
			for(; it != vIds.end(); ++it)
			{
				printf("%s\n", FormatMsgIdLine(**it, strTmp));
				delete *it;
			}
			printf("Total %ld\n", nTotal);
			return true;
		}

		if(str == "read")
		{
			// $msg read 10203040
			if(n != 3)
			{
				puts("Invalid parameters count.\n");
				PrintHelpMessage();
				return false;
			}
			const CZorgeString& strMsgId = vTokens[2];
			CMessage* p = g_pStorage->ReadMessage(g_OpenNumDev, strMsgId);
			if(p == 0)
			{
				printf("Failed to load message id %s\n", (const char*) strMsgId);
				return false;
			}
			printf("Status : %s\n", MessageStatusToString(p->m_Record.m_Data.m_nStatus));
			//printf("time_t = %ld\n", p->m_Record.m_nTimeCreated);
			CZorgeString strTmp;
			printf("Created : %s\n", TimeTtoString(UtcToLocal(p->m_Record.m_Data.m_nTimeCreated), strTmp));
			printf("From num : %s\n", p->m_Record.m_Data.m_szFromNum);
			printf("Message : %s\n", (const char*)p->m_strText);
			delete p;
			return true;
		}

		if(str == "delete")
		{
			// $msg delete 10203040
			if(n != 3)
			{
				puts("Invalid parameters count.\n");
				PrintHelpMessage();
				return false;
			}
			const CZorgeString& strMsgId = vTokens.at(2);
			g_pStorage->DeleteMessage(g_OpenNumDev, strMsgId);
			puts("Message deleted.");
			return true;
		}

		if(str == "sendToken")
		{
			// $msg sendToken 101 102 CE82DD994DB44B0F921883FAC19205E1 Hello
			if(n != 6)
			{
				puts("Invalid parameters count.\n");
				PrintHelpMessage();
				return false;
			}

			const CZorgeString& strFromNum = vTokens.at(2);
			const CZorgeString& strToNum = vTokens.at(3);
			const CZorgeString& strToken = vTokens.at(4);
			CZorgeString strMsg = vTokens.at(5);
			if(strMsg == "\"\"")
				strMsg.EmptyData();

			CNumDev Num = { strFromNum, strToken };
			CZorgeString strMsgId;
			g_pStorage->SendMsg(Num, strToNum, strMsg, strMsgId);
			printf("Message id = %s\n", (const char*)strMsgId);
			return true;
		}

	}
	catch(ALL_ERRORS& e)
	{
		printf("Error: %s\n", e.what());
		return false;
	}

	puts("Not supported parameter.\n");
	PrintHelpMessage();
	return false;
}

bool ProcessRepoCmd(const TTokens& vTokens)
{
	try
	{
		size_t n = vTokens.size();
		if(n == 1)
		{
			puts("Invalid parameters count.\n");
			PrintHelpRepo();
			return false;
		}

		const CZorgeString& str = vTokens.at(1);
		if(str == "status")
		{
			printf("Repository status : %s\n", g_pStorage->GetStatusStr());
			return true;
		}

		if(str == "login")
		{
			// #repo login mykey123
			if(n != 3)
			{
				puts("Invalid parameters count.\n");
				PrintHelpRepo();
				return false;
			}

			const CZorgeString& strKey = vTokens.at(2);
			g_pStorage->Login(strKey);
			printf("Repository status : %s\n", g_pStorage->GetStatusStr());
			return true;
		}

		if(str == "init")
		{
			// #repo init mykey123
			if(n != 3)
			{
				puts("Invalid parameters count.\n");
				PrintHelpRepo();
				return false;
			}

			const CZorgeString& strKey = vTokens.at(2);
			g_pStorage->Init(strKey);
			printf("Repository status : %s\n", g_pStorage->GetStatusStr());
			return true;
		}
	}
	catch(ALL_ERRORS& e)
	{
		printf("Error: %s\n", e.what());
		return false;
	}

	puts("Not supported parameter.\n");
	PrintHelpRepo();
	return false;
}

bool ProcessNumberCmd(const TTokens& vTokens)
{
	try
	{
		size_t n = vTokens.size();
		if(n == 2 && !g_OpenNumDev.m_strNum.IsEmpty())
		{
			if(vTokens.at(1) == "close")
			{
				printf("Number '%s' closed.\n", (const char*)g_OpenNumDev.m_strNum);
				g_OpenNumDev.m_strNum.EmptyData();
				g_OpenNumDev.m_strPin.EmptyData();
				return true;
			}
		}

		if(n < 3)
		{
			puts("Invalid parameters count.");
			PrintHelpNumber();
			return false;
		}

		const CZorgeString& str = vTokens.at(1);
		const CZorgeString& strNum = vTokens.at(2);
		if(str == "add")
		{
			// #number add 10-10 1
			if(n != 4)
			{
				puts("Invalid parameter count.");
				PrintHelpNumber();
				return false;
			}

			const CZorgeString& strPin = vTokens.at(3);
			CZorgeString strNewNum;
			g_pStorage->AddNumber(strNum, strPin, strNewNum);
			printf("New number added : %s\n", (const char*)strNewNum);
			return true;
		}

		if(str == "find")
		{
			// #number find 10-10-10
			if(n == 3)
			{
				bool bFound = g_pStorage->FindNumber(strNum);
				printf("Number '%s' %s\n", (const char*)strNum, bFound ? "FOUND" : "Not found");
				return true;
			}
			puts("Invalid parameters count.");
			PrintHelpNumber();
			return false;
		}

		if(str == "del")
		{
			// #number del 10-10 1
			if(n == 4)
			{
				const CZorgeString& strPin = vTokens.at(3);
				g_pStorage->DeleteNumber(strNum, strPin);
				printf("Number '%s' deleted\n", (const char*)strNum);
				return true;
			}
			puts("Invalid parameters count.");
			PrintHelpNumber();
			return false;
		}

		if(str == "open")
		{
			// #number open 10-10 1
			if(n != 4)
			{
				puts("Invalid parameter count.");
				PrintHelpNumber();
				return false;
			}
			const CZorgeString& strPin = vTokens.at(3);
			g_pStorage->OpenNumber(strNum, strPin);

			g_OpenNumDev.m_strNum = strNum;
			g_OpenNumDev.m_strPin = strPin;

			printf("Open number '%s'\n", (const char*)g_OpenNumDev.m_strNum);
			return true;
		}
	}
	catch(ALL_ERRORS& e)
	{
		printf("Error: %s\n", e.what());
		return false;
	}

	puts("Not supported parameter.");
	PrintHelpNumber();
	return false;
}

bool ProcessPrint(const CZorgeString& strLine)
{
	const char* szOut = (const char*)strLine;
	szOut += 6; // "print "
	printf("%s\n", szOut);
	return true;
}

bool ProcessDiag()
{
	CZorgeString str;
	g_pStorage->Diagnostics(str);
	printf("%s\n", (const char*)str);
	return true;
}

bool ProcessToken(const TTokens& vTokens)
{
	bool bNumOpen = !g_OpenNumDev.m_strNum.IsEmpty();

	try
	{
		size_t n = vTokens.size();
		if(n == 1)
		{
			puts("Invalid parameters count.\n");
			PrintHelpRepo();
			return false;
		}

		CZorgeString strToken;
		const CZorgeString& str = vTokens.at(1);
		if(str == "get")
		{
			if(!bNumOpen)
			{
				puts("Number is not open.");
				return false;
			}

			strToken = g_pStorage->GetNumToken(g_OpenNumDev);
			printf("%s\n", (const char*)strToken);
			return true;
		}

		if(str == "new")
		{
			if(!bNumOpen)
			{
				puts("Number is not open.");
				return false;
			}

			strToken = g_pStorage->NewNumToken(g_OpenNumDev);
			printf("%s\n", (const char*)strToken);
			return true;
		}
		if(str == "delete")
		{
			if(!bNumOpen)
			{
				puts("Number is not open.");
				return false;
			}

			g_pStorage->DeleteNumToken(g_OpenNumDev);
			puts("Token deleted.");
			return true;
		}

		if(str == "check")
		{
			if(n != 4)
				throw CZorgeError("Invalid parameter count. Expected : token check <num> <token>");

			CNumDev Num = { vTokens.at(2), vTokens.at(3) };
			bool bOK = g_pStorage->CheckNumToken(Num);
			if(bOK)
				puts("Token valid.");
			else
				puts("Token not valid.");
			return true;
		}
	}
	catch(ALL_ERRORS& e)
	{
		printf("Error: %s\n", e.what());
		return false;
	}

	puts("Not supported parameter.\n");
	PrintHelpRepo();
	return false;
}

bool ProcessInput(const CZorgeString& strLine)
{
	TTokens vTokens;
	if(strLine.Split(' ', vTokens) == 0)
		return true;

	const CZorgeString& strCmd = vTokens.at(0);
	if(strCmd == "repo")
		return ProcessRepoCmd(vTokens);
	else if(strCmd == "number" || strCmd == "num")
		return ProcessNumberCmd(vTokens);
	else if(strCmd == "message" || strCmd == "msg")
		return ProcessMessageCmd(vTokens);
	else if(strCmd == "print")
		return ProcessPrint(strLine);
	else if(strCmd == "diag")
		return ProcessDiag();
	else if(strCmd == "token")
		return ProcessToken(vTokens);

	printf("Not supported command '%s'\n", (const char*)strCmd);
	return false;
}

extern int Test(const CZorgeString& strRoot, int nTotalIters);
extern int Test_Thread(CStorage*, bool);

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
int main(int argc, char **argv)
{
	CSetSignalHandler SignaHandler;

	CZorgeString strStorage;
	char* szRoot = getenv(ZSMS_STORAGE_PATH);
	if(szRoot)
	{
		strStorage = szRoot;
		if(!strStorage.IsLastChar('/'))
			strStorage += "/";
	}

	puts("Zorge SMS shell.");
	if(argc == 2)
	{
		strStorage = argv[1];
		if(strcmp(argv[1], "/?") == 0)
		{
			puts("Usage : ./zss <path to storage folder>");
			puts("\nWhen started without parameter, folder can be set by ZSMS_STORAGE_PATH.");
			puts("export ZSMS_STORAGE_PATH=/tmp/folder1/folder2\n\n\n");
			return 1;
		}
	}

	if(argc > 2)
	{
		puts("Invalid command line parameter. Expected: ./zss [/path/to/storage/folder]");
		return 2; // Invalid command line parameter
	}

	if(strStorage.IsEmpty())
	{
		puts("\nEnvironmental variable ZSMS_STORAGE_PATH not defined and command line parameter is missing.");
		puts("To provide path to storage use:");
		puts("export ZSMS_STORAGE_PATH=/tmp/folder1/folder2 or");
		puts("./zss /tmp/folder1/folder2\n\n");
		return 2; // Invalid command line parameter
	}

	try
	{
		g_pStorage = new CStorage();
		g_pStorage->SetRoot(strStorage);
		printf("Root : %s\n", (const char*)strStorage);
		printf("Status : %s\n", g_pStorage->GetStatusStr());

		CZorgeString strLine;
		while(1)
		{
			if(g_OpenNumDev.m_strNum.IsEmpty())
				printf("\n#");
			else
				printf("\n[%s]$", (const char*)g_OpenNumDev.m_strNum);

			GetLineTerminal(strLine);
			if(strLine.IsEmpty())
				continue;
			if(strLine == "exit")
				break;
			if(strLine == "help" || strLine == "?")
			{
				PrintHelp();
				continue;
			}
			else if(strLine.Left(4) == "test")
			{
				int nIters = 1;
				TTokens vTokens;
				int n = strLine.Split(' ', vTokens);
				if(n == 2)
					nIters = atoi((const char*)vTokens[1]);
				else if(n != 1)
				{
					puts("Invalid parameter count.");
					continue;
				}
				if(nIters < 1)
				{
					printf("Invalid value for parameter. %d < 1\n", nIters);
					continue;
				}

				Test(strStorage, nIters);
				continue;
			}
			else if(strLine == "thread")
			{
				Test_Thread(g_pStorage, true);
				continue;
			}
			ProcessInput(strLine);
		}
	}
	catch(ALL_ERRORS& e)
	{
		printf("Error : %s\n", e.what());
	}
	if(g_pStorage)
		delete g_pStorage;

	puts("The end.");
	return 0;
}
