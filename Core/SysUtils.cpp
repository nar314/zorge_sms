#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/resource.h>
#include <termios.h>

#include "SysUtils.h"
#include "StrUtils.h"

#include <map>
using namespace std;

constexpr unsigned long int g_nEmptyBufferSize = 64 * 1024;
unsigned char* g_szEmptyBuffer = 0; // buffer used for purge
static long g_nMaxOpenFiles = 0;

void SetMaxDescriptors();

struct CRunOnce
{
    CRunOnce()
    {
        srand((unsigned int)time(0));

    	g_szEmptyBuffer = new unsigned char[g_nEmptyBufferSize];
    	memset((void*)g_szEmptyBuffer, 0, g_nEmptyBufferSize);
    	for(unsigned long int i = 0; i < g_nEmptyBufferSize; ++i)
    		g_szEmptyBuffer[i] = GetRandomByte();

        if(sizeof(size_t) != 8)
        {
            CZorgeString strMsg;
            strMsg.Format("size_t has to be 8 bites. sizeof(size_t)=%d\n", sizeof(size_t));
            puts((const char*)strMsg);
            throw CZorgeAssert(strMsg);
        }

        SetMaxDescriptors();
        GetMaxOpenFiles();
        printf("Max open files = %ld\n", g_nMaxOpenFiles);
    }
} g_Once;

//---------------------------------------------------------
//
//---------------------------------------------------------
unsigned char GetRandomByte()
{
    return (unsigned char)rand() % 255;
}

/*
//---------------------------------------------------------
//
//---------------------------------------------------------
unsigned char GetRandomByte(unsigned char chMax)
{
    if(chMax == 0)
        return 0;

    unsigned int i = 0;
    for(i = 0; i < 1000; i++)
    {
        unsigned char ch = (unsigned char)rand() % 255;
        if(ch <= chMax && ch != 0)
            return ch;
    }
    return chMax;
}

//---------------------------------------------------------
//
//---------------------------------------------------------
void InitBufferWithRandomData(unsigned char* pBuffer, size_t nBufLen)
{
    unsigned int i = 0;
    for(i = 0; i < nBufLen; i++)
    {
        unsigned char ch = (unsigned char)rand() % 255;
        pBuffer[i] = ch;
    }
}

*/

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int IsFileExist(const char* szFile)
{
    struct stat Buf;
    if (stat(szFile, &Buf) != 0)
    {
        // Failed to get attributes
        return NOT_EXIST;
    }
    else
    {
        if((Buf.st_mode & S_IFDIR) != 0)
            return FOLDER_EXIST; // it is a folder
    }

    return FILE_EXIST;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int	Unlink(const CZorgeString& strPathToFile)
{
	int nRc = 0;

	// I give 1 second to delete file
	for(int nTimes = 1; nTimes < 5; ++nTimes)
	{
		if(unlink((const char*)strPathToFile) == 0)
			return 0;

		nRc = errno;
		if(nRc == EAGAIN)
			ZorgeSleep(200); // 200 ms
		else
			break;
	}

	printf("Warning. Unlink failed with %d, %s\n", nRc, (const char*)strPathToFile);
	return nRc;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int	RmDir(const CZorgeString& strPathToFile)
{
	int nRc = 0;

	// I give 1 second to delete file
	for(int nTimes = 1; nTimes < 5; ++nTimes)
	{
		if(rmdir((const char*)strPathToFile) == 0)
			return 0;

		nRc = errno;
		if(nRc == EAGAIN)
			ZorgeSleep(200); // 200 ms
		else
			break;
	}

	return nRc;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int Rename(const CZorgeString& strCur, const CZorgeString& strNew)
{
	int nRc = 0;

	// I give 1 second to delete file
	for(int nTimes = 1; nTimes < 5; ++nTimes)
	{
		if(rename((const char*)strCur, (const char*)strNew) == 0)
			return 0;

		nRc = errno;
		if(nRc == EAGAIN)
			ZorgeSleep(200); // 200 ms
		else
			break;
	}

	return nRc;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int MkDir(const CZorgeString& strPath)
{
	int nRc = mkdir((const char*)strPath, S_IRWXU);
	if(nRc == -1 && errno == EEXIST)
		nRc = 0;

	if(nRc != 0)
		nRc = errno;

	return nRc;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void DeleteFilesTree(const char* szRoot, int nLevel)
{
	DIR* pDir = opendir(szRoot);
	if(pDir == 0)
	{
		int nErr = errno;
		CZorgeString strErr;
		strErr.Format("Failed to open dir '%s', errno = %d", szRoot, nErr);
		throw CZorgeError(strErr);
	}

	struct dirent* pObj = 0;
	while((pObj = readdir(pDir)) != 0)
	{
		CZorgeString strNewFolder(szRoot);
		if(strNewFolder.GetLastChar() != '/')
			strNewFolder += "/";

		const char* szName = pObj->d_name; // 256 chars is a limit !
		if(!strcmp(szName, "..") || !strcmp(szName, "."))
			continue;

		strNewFolder += szName;
		if(pObj->d_type == DT_DIR)
		{
			try
			{
				DeleteFilesTree((const char*)strNewFolder, nLevel + 1);
			}
			catch(ALL_ERRORS& e)
			{
				printf("Error : %s\n", e.what()); // have to continue to prevent leak.
			}
		}
		else
		{
			// Are you sure it is a file ?
			//printf("->%s", (const char*)strNewFolder);
			int n = Unlink(strNewFolder);
			if(n != 0)
				printf("Failed to delete '%s', errno = %d\n", (const char*)strNewFolder, n);
		}
	}
	if(nLevel > 0)
		RmDir(szRoot);

	closedir(pDir);
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bool IsFolderEmpty(const char* szRoot)
{
	DIR* pDir = opendir(szRoot);
	if(pDir == 0)
	{
		int nErr = errno;
		CZorgeString strErr;
		strErr.Format("Failed to open dir '%s', errno = %d", szRoot, nErr);
		throw CZorgeError(strErr);
	}

	struct dirent* pObj = 0;
	while((pObj = readdir(pDir)) != 0)
	{
		const char* szName = pObj->d_name; // 256 chars is a limit !
		if(!strcmp(szName, "..") || !strcmp(szName, "."))
			continue;

		closedir(pDir);
		return false; // Folder is not empty
	}

	closedir(pDir);
	return true; // Folder is empty
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bool IsRepoEmpty(const char* szRoot)
{
	DIR* pDir = opendir(szRoot);
	if(pDir == 0)
	{
		int nErr = errno;
		CZorgeString strErr;
		strErr.Format("Failed to open dir '%s', errno = %d", szRoot, nErr);
		throw CZorgeError(strErr);
	}

	struct dirent* pObj = 0;
	while((pObj = readdir(pDir)) != 0)
	{
		const char* szName = pObj->d_name; // 256 chars is a limit !
		if(!strcmp(szName, "..") || !strcmp(szName, "."))
			continue;

		if(pObj->d_type != DT_DIR)
		{
			closedir(pDir);
			return false; // Repository is not empty
		}
	}

	closedir(pDir);
	return true; // Repository is empty
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bool StringToFile(const CZorgeString& strPath, const CZorgeString& strData)
{
	int nHandler = open((const char*)strPath, O_CREAT | O_TRUNC | O_RDWR, S_IREAD | S_IWRITE);
	if(nHandler == -1)
		return false;

	int  nWrote = 0;
	bool bSucceed = true;
	int  nRestLen = strData.GetLength();
	int	 nLenToWrite = nRestLen;

	const char* szData = (const char*)strData;

	while(nRestLen != 0)
	{
		//nLenToWrite = nRestLen > BUFF_SIZE ? BUFF_SIZE : nRestLen;
		nLenToWrite = nRestLen; // Do not limit to OS buffer.

		nWrote = write(nHandler, szData, nLenToWrite);
		if(nWrote == -1)
		{
			printf("StringToFile() failed to write. errno = %d\n", errno);
			bSucceed = false;
			break;
		}
		szData += nLenToWrite;
		nRestLen -= nLenToWrite;
	}

	close(nHandler);
	return bSucceed;
}

#define IO_BUFFER_SIZE 65536 // 64K
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bool FileToString(const CZorgeString& strPath, CZorgeString& strOut)
{
	strOut.EmptyData();
	int nHandler = open((const char*)strPath, O_RDONLY, S_IREAD);
	if(nHandler == -1)
		return false;

	char* szBuff = new char[IO_BUFFER_SIZE];
	int  nRead = 0;
	bool bSucceed = true;

	while(1)
	{
		nRead = read(nHandler, szBuff, IO_BUFFER_SIZE);
		if(nRead == -1)
		{
			strOut.EmptyData();
			bSucceed = false;
			break;
		}
		else if(nRead == 0)
			break;

		szBuff[nRead] = 0;
		strOut += szBuff;
	}

	close(nHandler);
	delete [] szBuff;
	return bSucceed;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int	EnumFolder(const CZorgeString& strFolder, TFsNames& vOut)
{
	vOut.clear();
	DIR* pDir = opendir((const char*)strFolder);
	if(pDir == 0)
	{
		int nErr = errno;
		CZorgeString strErr;
		strErr.Format("Failed to open dir '%s', errno = %d", (const char*)strFolder, nErr);
		throw CZorgeError(strErr);
	}

	struct dirent* pObj = 0;
	while((pObj = readdir(pDir)) != 0)
	{
		const char* szName = pObj->d_name; // 256 chars is a limit !
		if(!strcmp(szName, "..") || !strcmp(szName, "."))
			continue;

		vOut.push_back(CFsName(pObj->d_type == DT_DIR, pObj->d_name));
	}

	closedir(pDir);
	return vOut.size();
}

//---------------------------------------------------------
//
//---------------------------------------------------------
bool StringToFileAppend(const CZorgeString& strPath, const CZorgeString& strData)
{
	int nHandler = open((const char*)strPath, O_CREAT | O_RDWR, S_IREAD | S_IWRITE);
	if(nHandler == -1)
		return false;

	if(lseek(nHandler, 0, SEEK_END) == -1)
		throw CZorgeAssert("lseek failed.");

	int  nWrote = 0;
	bool bSucceed = true;
	int  nRestLen = strData.GetLength();
	int	 nLenToWrite = nRestLen;

	const char* szData = (const char*)strData;

	while(nRestLen != 0)
	{
		//nLenToWrite = nRestLen > BUFF_SIZE ? BUFF_SIZE : nRestLen;
		nLenToWrite = nRestLen; // Do not limit to OS buffer.

		nWrote = write(nHandler, szData, nLenToWrite);
		if(nWrote == -1)
		{
			printf("StringToFile() failed to write. errno = %d\n", errno);
			bSucceed = false;
			break;
		}
		szData += nLenToWrite;
		nRestLen -= nLenToWrite;
	}

	close(nHandler);
	return bSucceed;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int PurgeFile(const CZorgeString& strPath)
{
	const char* szPath = (const char*)strPath;
    struct stat Buf;
    if (stat(szPath, &Buf) != 0)
    {
        // Failed to get attributes
        return 0;
    }
    else
    {
        if((Buf.st_mode & S_IFDIR) != 0)
        {
            printf("Folder found instead of file '%s'\n", szPath);
            return 254;
        }
    }

    unsigned int nFileSize = Buf.st_size;
    if(nFileSize == 0)
    	return Unlink(strPath);

    // Purge it
    int nErrno = 0;
	int nHandler = open((const char*)strPath, O_WRONLY, S_IREAD | S_IWRITE);
	if(nHandler == -1)
	{
		nErrno = errno;
		printf("Failed to open file '%s', errno = %d\n", szPath, nErrno);
		return nErrno;
	}

	unsigned int nWrote = 0;
	unsigned int nRestLen = nFileSize;
	unsigned int nLenToWrite = nRestLen;

	while(nRestLen != 0)
	{
		nLenToWrite = nRestLen > g_nEmptyBufferSize ? g_nEmptyBufferSize : nRestLen;

		nWrote = write(nHandler, g_szEmptyBuffer, nLenToWrite);
		if(nWrote != nLenToWrite)
		{
			nErrno = errno;
			printf("Failed to write into file '%s', errno = %d\n", szPath, nErrno);
			break;
		}
		nRestLen -= nLenToWrite;
	}

	close(nHandler);
	if(nErrno == 0)
		return Unlink(strPath);
	else
		printf("PurgeFailed with errno = %d %s\n", nErrno, szPath);

	return nErrno;
}

//---------------------------------------------------------
//
//---------------------------------------------------------
long GetMaxOpenFiles()
{
	if(g_nMaxOpenFiles == 0)
		g_nMaxOpenFiles = sysconf(_SC_OPEN_MAX);
	return g_nMaxOpenFiles;
}

// time_t t1 = GetUtcNow();
// printf("%s", asctime(localtime(&t1)));
//---------------------------------------------------------
//
//---------------------------------------------------------
time_t GetUtcNow()
{
	time_t nLocal = time(0);
	struct tm* pTime = gmtime(&nLocal);
	time_t nUtc = mktime(pTime);
	return nUtc;
}

//---------------------------------------------------------
//
//---------------------------------------------------------
time_t UtcToLocal(time_t utcTime)
{
	struct tm* pTime = localtime(&utcTime);
	pTime->tm_isdst = -1; // Yes for date light saving.
	time_t nLocal = mktime(pTime) + pTime->tm_gmtoff;
	return nLocal;
}

//------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------
void TimeNowToString(CZorgeString& strOut)
{
	time_t nLocal = time(0);
	struct tm* pTime = localtime(&nLocal);

	char szTime[32];
	snprintf(szTime, 32, "%02d:%02d:%02d", pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
	strOut = szTime;
}

//------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------
const char* TimeToString(time_t nLocal, CZorgeString& strOut)
{
	if(nLocal == 0)
		return "";
	struct tm* pTime = localtime(&nLocal);

	char szTime[32];
	snprintf(szTime, 32, "%02d:%02d:%02d", pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
	strOut = szTime;
	return (const char*)strOut;
}

//---------------------------------------------------------
// https://stackoverflow.com/questions/669438/how-to-get-memory-usage-at-runtime-using-c
//---------------------------------------------------------
unsigned long GetResMemUsage_KB()
{
	int nSelf = RUSAGE_SELF;
	struct rusage Usage;

	if(getrusage(nSelf, &Usage) == 0)
		return Usage.ru_maxrss;

	return 0;
}

//------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------
bool MemToFile(const CZorgeString& strPath, const CMemBuffer& Data)
{
	int nHandler = open((const char*)strPath, O_CREAT | O_TRUNC | O_RDWR, S_IREAD | S_IWRITE);
	if(nHandler == -1)
	{
		int n = errno;
		printf("MemToFile failed to open file %s, errno = %d\n", (const char*)strPath, n);
		return false;
	}

	int  nWrote = 0;
	bool bSucceed = true;
	int  nRestLen = Data.GetSizeData();
	int	 nLenToWrite = nRestLen;

	const char* szData = (const char*)Data.Get();
	while(nRestLen != 0)
	{
		//nLenToWrite = nRestLen > BUFF_SIZE ? BUFF_SIZE : nRestLen;
		nLenToWrite = nRestLen; // Do not limit to OS buffer.

		nWrote = write(nHandler, szData, nLenToWrite);
		if(nWrote == -1)
		{
			printf("MemToFile() failed to write. errno = %d, %d\n", errno, __LINE__);
			bSucceed = false;
			break;
		}
		szData += nLenToWrite;
		nRestLen -= nLenToWrite;
	}

	close(nHandler);
	return bSucceed;
}

static bool GetFileSize(const CZorgeString& strPath, unsigned int& nSize)
{
	struct stat Buf;
	int n = stat((const char*)strPath, &Buf);
	if (n != 0)
		return false;

	if((Buf.st_mode & S_IFDIR) != 0)
		return false;

	nSize = Buf.st_size;
	return true;
}

//------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------
bool FileToMem(const CZorgeString& strPath, CMemBuffer& Out)
{
	unsigned int nFileSize = 0;
	if(!GetFileSize(strPath, nFileSize))
		return false;

	int nHandler = open((const char*)strPath, O_RDONLY, S_IREAD);
	if(nHandler == -1)
	{
		int n = errno;
		printf("FileToMem failed to open file %s, errno = %d\n", (const char*)strPath, n);
		return false;
	}

	Out.Allocate(nFileSize);
	unsigned char* szBuff = Out.Get();

	int  nRead = 0;
	unsigned int nLeft = nFileSize;
	bool bSucceed = true;

	while(nLeft > 0)
	{
		nRead = read(nHandler, szBuff, nLeft);
		if(nRead == -1)
		{
			bSucceed = false;
			break;
		}
		else if(nRead == 0)
			break;

		nLeft -= nRead;
		szBuff += nRead;
	}

	Out.SetSizeData(nFileSize);
	close(nHandler);
	return bSucceed;
}

//------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------
void ZorgeSleep(unsigned int nMs)
{
	usleep(nMs * 1000);
}

//------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------
void SetMaxDescriptors()
{
	int nResId = RLIMIT_NOFILE;
	struct rlimit Limit;
	if(getrlimit(nResId, &Limit) == -1)
	{
		int n = errno;
		printf("getrlimit() failed with errno %d\n", n);
		return;
	}
	rlim_t nFromSoftLimit = Limit.rlim_cur;

	// Set max to the hard limit.
	Limit.rlim_cur = Limit.rlim_max;

	if(setrlimit(nResId, &Limit) == -1)
	{
		int n = errno;
		printf("setrlimit() failed with errno %d\n", n);
		return;
	}

	printf("Soft limit for open handles set from %ld to %ld\n", nFromSoftLimit, Limit.rlim_cur);
}

//------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------
void EnterPassword(CZorgeString& strOut)
{
	strOut.EmptyData();
	int nBufSize = 2048; // Have you ever see such long passwords ?
	char* pBuf = new char[nBufSize + 1];

	static struct termios OldTerm, NewTerm;
	tcgetattr(STDIN_FILENO, &OldTerm);
	NewTerm = OldTerm;
	NewTerm.c_lflag &= ~(ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &NewTerm);
	char ch = 0;
	int nPos = 0;
	// There is a problem when Ctrl-C comes.
	while ((ch = getchar()) != '\n' && ch != EOF && nPos < nBufSize)
		pBuf[nPos++] = ch;
	pBuf[nPos] = '\0';
	tcsetattr(STDIN_FILENO, TCSANOW, &OldTerm);

	strOut = pBuf;
	delete [] pBuf;
}

//------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------
void MkPath(const CZorgeString& strPath)
{
	CZorgeString strCurPath("/");
	TTokens vTokens;
	strPath.Split('/', vTokens);
	for(const CZorgeString& str : vTokens)
	{
		if(str.IsEmpty() || str == "/")
			continue;
		strCurPath += str;
		strCurPath += "/";
		int n = MkDir(strCurPath);
		if(n != 0)
		{
			CZorgeString strErr;
			strErr.Format("Failed to create folder '%s', errno=%d, full path '%s'", (const char*)strCurPath, n, (const char*)strPath);
			throw CZorgeError(strErr);
		}
	}
}

/*
void F()
{
	try
	{
		CZorgeString s("/tmp/F1/Last");
		MkPath(s);
	}
	catch(ALL_ERRORS& e)
	{
		printf("%s\n", e.what());
	}
}

struct CC1
{
    CC1()
    {
    	F();
    	puts("OK");
    }
} z1;
*/
