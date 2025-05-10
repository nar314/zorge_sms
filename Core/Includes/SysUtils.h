// SysUtils.h

#ifndef ZORGE_SYS_UTILS_H
#define ZORGE_SYS_UTILS_H

#include <time.h>
#include <vector>
#include "ZorgeError.h"
#include "MemBuffer.h"

struct CFsName
{
	bool 			m_bFolder;
	CZorgeString	m_strName;
	CFsName(const CFsName& Copy)
	{
		m_bFolder = Copy.m_bFolder;
		m_strName = Copy.m_strName;
	}
	CFsName(bool b, const char* szName)
	{
		m_bFolder = b;
		m_strName = szName;
	}
};
typedef vector<CFsName> TFsNames;

#define NOT_EXIST (-1)
#define FOLDER_EXIST 1
#define FILE_EXIST 2

int 	MkDir(const CZorgeString&);
void 	MkPath(const CZorgeString& strPath);
int		RmDir(const CZorgeString&);
int 	Rename(const CZorgeString&, const CZorgeString&);
int		Unlink(const CZorgeString&);
int 	PurgeFile(const CZorgeString&);

int 	IsFileExist(const char*);
void 	DeleteFilesTree(const char*, int);
bool 	IsFolderEmpty(const char*);
bool 	IsRepoEmpty(const char*);

bool	StringToFile(const CZorgeString&, const CZorgeString&);
bool 	StringToFileAppend(const CZorgeString&, const CZorgeString&);
bool 	FileToString(const CZorgeString&, CZorgeString&);
bool 	FileToMem(const CZorgeString&, CMemBuffer&);
bool 	MemToFile(const CZorgeString&, const CMemBuffer&);

int		EnumFolder(const CZorgeString&, TFsNames&);

long 	GetMaxOpenFiles();
unsigned char GetRandomByte();

time_t 		GetUtcNow();
time_t 		UtcToLocal(time_t utcTime);
void 		TimeNowToString(CZorgeString& strOut);
const char* TimeToString(time_t nLocal, CZorgeString& strOut);

unsigned long GetResMemUsage_KB();

void 	ZorgeSleep(unsigned int);
void 	EnterPassword(CZorgeString& strOut);

#endif
