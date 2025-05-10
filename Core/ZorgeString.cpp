#include "ZorgeString.h"
#include <atomic>

#include <string.h>
#include <stdio.h>
#include <ctype.h>	// for isspace, toupper, tolower
#include <stdlib.h> // for atoi
#include <stdarg.h> // for va_arg

std::atomic<unsigned int> g_CZorgeString = 0;
std::atomic<unsigned int> g_CZorgeString_max = 0;

struct CZorgeStringCounter
{
	~CZorgeStringCounter()
	{
		printf("g_CZorgeString = %u (%u)\n", (unsigned int)g_CZorgeString, (unsigned int)g_CZorgeString_max);
	}
	static void Add()
	{
		g_CZorgeString++;
		g_CZorgeString_max++;
	}
} g_CZorgeStringCounter;

void DiagCZorgeString(CZorgeString& strOut)
{
	CZorgeString str;
	str.Format("CZorgeString.\tCur=%d, Max=%d\n", (unsigned int)g_CZorgeString, (unsigned int)g_CZorgeString_max);
	strOut += str;
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
CZorgeString::CZorgeString()
{
	CZorgeStringCounter::Add();
	m_szData = 0;
	m_nAlloc = m_nUsed = m_nAllocStep = 0;
	m_szData = 0;
}

CZorgeString::CZorgeString(const char* szStr)
{
	CZorgeStringCounter::Add();
	m_szData = 0;
	m_nAlloc = m_nUsed = m_nAllocStep = 0;
	m_szData = 0;
	SetData(szStr, 0);
}

CZorgeString::CZorgeString(const CZorgeString& strSource)
{
	CZorgeStringCounter::Add();
	m_nAllocStep = 0; // don't copy allocation step value
	m_nAlloc = strSource.m_nAlloc;
	m_nUsed = strSource.m_nUsed;

	m_szData = 0;

	if(m_nAlloc)
	{
		m_szData = new unsigned char[m_nAlloc];
		memcpy(m_szData, strSource.m_szData, m_nAlloc); // copy including trailing 0x00
	}
}

CZorgeString::CZorgeString(unsigned int nPreAllocateBytes)
{
	CZorgeStringCounter::Add();
	m_szData = 0;
	m_nAlloc = m_nUsed = m_nAllocStep = 0;
	m_szData = 0;
	GetBuffer(nPreAllocateBytes);
}

CZorgeString::~CZorgeString()
{
	g_CZorgeString--;
	if(m_szData)
		delete [] m_szData;
}

void CZorgeString::AppendData(const char* szData, unsigned int nLen)
{
	if(szData == 0)
		return;

	if(m_nAlloc == 0)
	{
		SetData(szData, nLen);
		return;
	}

	unsigned int nLenData = (unsigned int)strlen(szData);
	unsigned int nFree = m_nAlloc - m_nUsed - 1;
	if(nFree > nLenData)
	{
		if(nLenData == 1)
			m_szData[m_nUsed] = *szData;
		else
			memcpy(&m_szData[m_nUsed], szData, nLenData);
		m_nUsed += nLenData;
		m_szData[m_nUsed] = 0;
	}
	else
	{
		// re-allocate
		unsigned int nAllocCalc = ((unsigned int)((m_nUsed + nLenData + m_nAllocStep)/16) + 1) * 16;
		m_nAlloc = nAllocCalc;
		unsigned char* szNew = new unsigned char[m_nAlloc];
		memcpy(szNew, m_szData, m_nUsed);
		memcpy(szNew + m_nUsed, szData, nLenData);
		m_nUsed += nLenData;
		szNew[m_nUsed] = 0;

		delete [] m_szData;
		m_szData = szNew;
	}
}

//-----------------------------------------------------------------
// Get string as pointer to raw data buffer
//-----------------------------------------------------------------
CZorgeString::operator const char*() const
{
	if(m_szData == 0)
		return "";

	return (const char*)m_szData;
}

//-----------------------------------------------------------------
// Get length of the string in bytes.
//-----------------------------------------------------------------
unsigned int CZorgeString::GetLength() const
{
	return m_nUsed;
}

//-----------------------------------------------------------------
// Get allocated length of the string in bytes.
//-----------------------------------------------------------------
unsigned int CZorgeString::GetAllocLength() const
{
	return m_nAlloc;
}

//-----------------------------------------------------------------
// set memory allocation minimum step
//-----------------------------------------------------------------
void CZorgeString::SetAllocStep(unsigned int nStep)
{
	m_nAllocStep = nStep;
}

//-----------------------------------------------------------------
// assign from another string
//-----------------------------------------------------------------
CZorgeString& CZorgeString::operator=(const CZorgeString& strSource)
{
	if(strSource == *this)
		return *this;

	m_nAllocStep = 0; // don't copy allocation step value
	m_nAlloc = strSource.m_nAlloc;
	m_nUsed = strSource.m_nUsed;

	if(m_szData)
		delete [] m_szData, m_szData = 0;

	if(m_nAlloc)
	{
		m_szData = new unsigned char[m_nAlloc];
		memcpy(m_szData, strSource.m_szData, m_nAlloc); // copy including trailing 0x00
	}

	return *this;
}

//-----------------------------------------------------------------
// append from another string
//-----------------------------------------------------------------
CZorgeString& CZorgeString::operator+=(const CZorgeString& strSource)
{
	AppendData((const char*)strSource, 0);
	return *this;
}

//-----------------------------------------------------------------
// append constant to the string
//-----------------------------------------------------------------
CZorgeString& CZorgeString::operator+=(const char* szStr)
{
	AppendData(szStr, 0);
	return *this;
}

//-----------------------------------------------------------------
// append char to the string
//-----------------------------------------------------------------
CZorgeString& CZorgeString::operator+=(const char ch)
{
	char szTmp[2];
	szTmp[0] = ch;
	szTmp[1] = 0;
	AppendData(szTmp, 1);
	return *this;
}

//-----------------------------------------------------------------
// Empty string. Data buffer will be released if not shared
//-----------------------------------------------------------------
void CZorgeString::Empty()
{
	m_nAllocStep = m_nAlloc = m_nUsed = 0;
	if(m_szData)
		delete [] m_szData, m_szData = 0;
}

//-----------------------------------------------------------------
// Empty data buffer. Set EOL as first char
//-----------------------------------------------------------------
void CZorgeString::EmptyData()
{
	m_nUsed = 0;
	if(m_szData)
		m_szData[0] = 0;
}

//-----------------------------------------------------------------
// return true if data buffer is empty.
//-----------------------------------------------------------------
bool CZorgeString::IsEmpty() const
{
	return (m_nUsed == 0);
}

//-----------------------------------------------------------------
// dump string data
//-----------------------------------------------------------------
void CZorgeString::Dump()
{
//	printf("[%X %X] m_nAlloc = %d, m_nUsed = %d, m_nRef = %d, m_szData = %s\n", this, m_pData,
//	(unsigned int)m_pData->m_nAlloc, m_pData->m_nUsed, m_pData->m_nRef, m_pData->m_szData);
}

//-----------------------------------------------------------------
// compare string with string constant
//-----------------------------------------------------------------
bool CZorgeString::operator==(const char* szData) const
{
	if(m_szData == 0)
		return false;

	return (strcmp((const char*)m_szData, szData) == 0);
}

//-----------------------------------------------------------------
// compare string with another string
//-----------------------------------------------------------------
bool CZorgeString::operator==(const CZorgeString& strString) const
{
	if(m_szData == 0)
		return false;

	if(strString.m_szData == 0)
		return false;

	return (strcmp((const char*)m_szData, (const char*)(strString.m_szData)) == 0);
}

//-----------------------------------------------------------------
// compare string with another string
//-----------------------------------------------------------------
bool CZorgeString::operator!=(const CZorgeString& strString) const
{
	if(m_szData == 0 && strString.m_szData == 0)
		return false;

	if(m_szData == 0)
		return true;

	if(strString.m_szData == 0)
		return true;

	if(strString.m_szData == 0)
		return true;

	return (strcmp((const char*)m_szData, (const char*)(strString.m_szData)) != 0);
}

//-----------------------------------------------------------------
// compare string with another string
//-----------------------------------------------------------------
bool CZorgeString::operator!=(const char* szStr) const
{
	if(m_szData == 0 && szStr == 0)
		return false;

	if(m_szData == 0)
		return true;

	if(szStr == 0)
		return true;

	return (strcmp((const char*)m_szData, szStr) != 0);
}

bool CZorgeString::operator<(const CZorgeString& strString) const
{
	if(m_szData == 0)
		return true;

	if(strString.m_szData == 0)
		return true;

	if(strString.m_szData == 0)
		return true;

	return (strcmp((const char*)m_szData, (const char*)(strString.m_szData)) < 0);
}

//-----------------------------------------------------------------
// return true if first char is 'ch'
//-----------------------------------------------------------------
bool CZorgeString::IsFirstChar(char ch) const
{
	if(m_nUsed == 0)
		return false;

	if(m_szData == 0)
		return false;

	return (m_szData[0] == ch);
}

//-----------------------------------------------------------------
// return true if last char is 'ch'
//-----------------------------------------------------------------
bool CZorgeString::IsLastChar(char ch) const
{
	if(m_nUsed == 0)
		return false;

	if(m_szData == 0)
		return false;

	return (m_szData[m_nUsed - 1] == ch);
}

//-----------------------------------------------------------------
// Get last char. You will have an issue with the Unicode
//-----------------------------------------------------------------
char CZorgeString::GetLastChar() const
{
    if(m_nUsed == 0)
        return false;

    if(m_szData == 0)
        return false;

    return m_szData[m_nUsed - 1];
}

//-----------------------------------------------------------------
// trim trailing white spaces
//-----------------------------------------------------------------
void CZorgeString::TrimRight()
{
	if(m_nUsed == 0)
		return;

	if(m_szData == 0)
		return;

	unsigned char* ch = &m_szData[m_nUsed - 1];
	unsigned int nTotalFound(0);
	for(; *ch != 0; --ch)
	{
		if(isspace(*ch))
		{
			*ch = 0;
			++nTotalFound;
		}
		else
			break;
	}

	if(nTotalFound)
		m_nUsed -= nTotalFound;
}

//-----------------------------------------------------------------
// trim leading white spaces
//-----------------------------------------------------------------
void CZorgeString::TrimLeft()
{
	if(m_szData == 0)
		return;

	if(m_nUsed == 0)
		return;

	unsigned char* ch = &m_szData[0];
	unsigned int nTotalFound(0);
	for(; *ch != 0; ++ch)
	{
		if(isspace(*ch))
			++nTotalFound;
		else
			break;
	}

	if(nTotalFound)
	{
		m_nUsed -= nTotalFound;
		if(m_nUsed == 0)
		{
			m_szData[0] = 0;
			return;
		}

		unsigned char* pSrc = &m_szData[nTotalFound];
		unsigned char* pDest = &m_szData[0];

		memcpy(pDest, pSrc, m_nUsed);
		m_szData[m_nUsed] = 0;
	}
}

//-----------------------------------------------------------------
// trim leading and trailing white spaces
//-----------------------------------------------------------------
void CZorgeString::Trim()
{
	TrimLeft();
	TrimRight();
}

//-----------------------------------------------------------------
// Create new string from leading chars
//-----------------------------------------------------------------
CZorgeString CZorgeString::Left(unsigned int nPos) const
{
	if(nPos == 0 || IsEmpty())
        return CZorgeString();

	if(nPos >= m_nUsed)
        return CZorgeString(*this);

	char* szTmp = new char[nPos + 1];
	memcpy(szTmp, m_szData, nPos);
	szTmp[nPos] = 0;
    CZorgeString strOut(szTmp);
	delete [] szTmp;
	return strOut;
}

//-----------------------------------------------------------------
// Create new string from trailing chars
//-----------------------------------------------------------------
CZorgeString CZorgeString::Right(unsigned int nPos) const
{
	if(IsEmpty() || nPos >= m_nUsed)
        return CZorgeString();

	if(nPos == 0)
        return CZorgeString(*this);

	char* szTmp = new char[nPos + 1];
	memcpy(szTmp, &m_szData[m_nUsed - nPos], nPos);
	szTmp[nPos] = 0;
    CZorgeString strOut(szTmp);
	delete [] szTmp;
	return strOut;
}

//-----------------------------------------------------------------
// Create new string from the string
//-----------------------------------------------------------------
CZorgeString CZorgeString::Mid(unsigned int nPos, unsigned int nLen) const
{
	if(IsEmpty() || nPos >= m_nUsed || nLen == 0)
        return CZorgeString();

	int nFinalLen = nLen;
	if(nPos + nLen > m_nUsed)
		nFinalLen = m_nUsed - nPos;

	char* szTmp = new char[nFinalLen + 1];
	memcpy(szTmp, &m_szData[nPos], nFinalLen);
	szTmp[nFinalLen] = 0;
    CZorgeString strOut(szTmp);
	delete [] szTmp;
	return strOut;
}

//-----------------------------------------------------------------
// Find char in the string. Return -1 if not found. Index starts from 0
//-----------------------------------------------------------------
int	CZorgeString::Find(char ch) const
{
	if(m_nUsed == 0 || m_szData == 0 || ch == 0)
		return STR_NOT_FOUND;

	int nPos = 0;
	unsigned char* chPtr = m_szData;
	for(; *chPtr != 0; ++chPtr, ++nPos)
	{
		if(*chPtr == ch)
			return nPos;
	}
	return STR_NOT_FOUND;
}

//-----------------------------------------------------------------
// Find char in the string in reverse order.
// Return -1 if not found. Index starts from 0
//-----------------------------------------------------------------
int CZorgeString::ReverseFind(char ch) const
{
	if(m_nUsed == 0 || m_szData == 0 || ch == 0)
		return STR_NOT_FOUND;

	unsigned char* chPtr = m_szData + m_nUsed;
	unsigned char* szBegin = m_szData;
	for(; chPtr != szBegin; --chPtr)
	{
		if(*chPtr == ch)
			return (int)(chPtr - szBegin);
	}

	if(*szBegin == ch)
		return 0;

	return STR_NOT_FOUND;
}

//-----------------------------------------------------------------
// Find char in the string from position
// Return -1 if not found. Index starts from 0
//-----------------------------------------------------------------
int	CZorgeString::Find(char ch, unsigned int nPos) const
{
	if(m_nUsed == 0 || m_szData == 0 || ch == 0 || nPos >= m_nUsed)
		return STR_NOT_FOUND;

	int nRetPos = nPos;
	unsigned char* chPtr = m_szData + nPos;
	for(; *chPtr != 0; ++chPtr, ++nRetPos)
	{
		if(*chPtr == ch)
			return nRetPos;
	}
	return STR_NOT_FOUND;
}

//-----------------------------------------------------------------
// Find string in the string
// Return -1 if not found. Index starts from 0
//-----------------------------------------------------------------
int	CZorgeString::Find(const char* szStr) const
{
	if(m_nUsed == 0 || m_szData == 0 || *szStr == 0)
		return STR_NOT_FOUND;

	unsigned int nSourceLen = (unsigned int)strlen(szStr);
	if(nSourceLen == 0)
		return STR_NOT_FOUND;

	unsigned char* szPtr = m_szData;

    const char* szFound = strstr((const char*)szPtr, szStr);
    if(szFound == 0)
        return STR_NOT_FOUND;

    int nPos = (int)(szFound - (const char*)szPtr);
    return nPos;
}

//-----------------------------------------------------------------
// Find string in the string from position
// Return -1 if not found. Index starts from 0
//-----------------------------------------------------------------
int	CZorgeString::Find(const char* szStr, unsigned int nPos) const
{
	if(m_nUsed == 0 || m_szData == 0 || *szStr == 0 || nPos > m_nUsed)
		return STR_NOT_FOUND;

	unsigned int nSourceLen = (unsigned int)strlen(szStr);
	if(nPos + nSourceLen > m_nUsed)
		return STR_NOT_FOUND;

	unsigned char* szPtr = m_szData + nPos;
	unsigned int nDataLen = m_nUsed;
	unsigned int nLeft = nDataLen - nPos;
	int nCurPos = nPos;
	for(; *szPtr != 0; ++nCurPos)
	{
		if(nLeft < nSourceLen)
			return STR_NOT_FOUND;

		if(strncmp((const char*)szPtr, szStr, nSourceLen) == 0)
			break;
		else
		{
			++szPtr;
			--nLeft;
		}
	}
	return nCurPos;
}

//-----------------------------------------------------------------
// Make string upper cased
//-----------------------------------------------------------------
void CZorgeString::MakeUpper()
{
	if(m_nUsed == 0)
		return;

	if(m_szData == 0)
		return;

	unsigned char* ch = m_szData;
	for(; *ch != 0; ++ch)
		*ch = (unsigned char)toupper(*ch);
}

//-----------------------------------------------------------------
// Make string lower cased
//-----------------------------------------------------------------
void CZorgeString::MakeLower()
{
	if(m_nUsed == 0)
		return;

	if(m_szData == 0)
		return;

	unsigned char* ch = m_szData;
	for(; *ch != 0; ++ch)
		*ch = (unsigned char)tolower(*ch);
}

//-----------------------------------------------------------------
// format string according to szFormat
//-----------------------------------------------------------------
void CZorgeString::Format(const char* szFormat, ...)
{
	va_list ArgList;
	va_start(ArgList, szFormat);
	FormatPr(szFormat, ArgList);
	va_end(ArgList);
}

//-----------------------------------------------------------------
// private. Does format
//-----------------------------------------------------------------
void CZorgeString::FormatPr(const char* szFormat, va_list argList)
{
    va_list argListSave;
    va_copy(argListSave, argList);

	// make a guess at the maximum length of the resulting string
	int nMaxLen = 0;
	const char* sz = szFormat;
	for(; *sz != 0; ++sz)
	{
		// handle '%' character, but watch out for '%%'
		if(*sz != '%' || *(++sz) == '%')
		{
			nMaxLen += (int)strlen(sz);
			continue;
		}

		int nItemLen = 0;

		// handle '%' character with format
		int nWidth = 0;
		for (; *sz != 0; ++sz)
		{
			// check for valid flags
			if(*sz == '#')
				nMaxLen += 2;   // for '0x'
			else if(*sz == '*')
				nWidth = va_arg(argList, int);
			else if(*sz == '-' || *sz == '+' || *sz == '0' || *sz == ' ')
				;
			else // hit non-flag character
				break;
		}
		// get width and skip it
		if(nWidth == 0)
		{
			// width indicated by
			nWidth = atoi(sz);
			for (; *sz != 0 && isdigit(*sz); ++sz);
		}

		int nPrecision = 0;
		if(*sz == '.')
		{
			// skip past '.' separator (width.precision)
			++sz;

			// get precision and skip it
			if(*sz == '*')
			{
				nPrecision = va_arg(argList, int);
				++sz;
			}
			else
			{
				nPrecision = atoi(sz);
				for(; *sz != 0 && isdigit(*sz); sz++);
			}
		}

		// should be on type modifier or specifier
		switch(*sz)
		{
			// modifiers that affect size
			case 'h':
				++sz;
				break;
			// modifiers that do not affect size
			case 'F':
			case 'N':
			case 'L':
				++sz;
				break;
		}

		// now should be on specifier
		switch(*sz)
		{
			case 'c':
			case 'C':
				nItemLen = 2;
				va_arg(argList, int);
				break;

			case 's':
				{
					const char* pstrNextArg = va_arg(argList, const char*);
					if(pstrNextArg == 0)
					   nItemLen = 6;  // "(null)"
					else
					{
					   nItemLen = (int)strlen(pstrNextArg);
					   nItemLen = max(1, nItemLen);
					}
				}
				break;
		}

		// adjust nItemLen for strings
		if(nItemLen != 0)
		{
			if(nPrecision != 0)
				nItemLen = min(nItemLen, nPrecision);
			nItemLen = max(nItemLen, nWidth);
		}
		else
		{
			switch (*sz)
			{
				// integers
				case 'd':
				case 'i':
				case 'u':
				case 'x':
				case 'X':
				case 'o':
				case 'l':
					va_arg(argList, int);
					nItemLen = 32;
					nItemLen = max(nItemLen, nWidth+nPrecision);
					break;

				case 'e':
				case 'g':
				case 'G':
					va_arg(argList, double);
					nItemLen = 128;
					nItemLen = max(nItemLen, nWidth+nPrecision);
					break;

				case 'f':
					va_arg(argList, double);
					nItemLen = 128; // width isn't truncated
					// 312 == strlen("-1+(309 zeroes).")
					// 309 zeroes == max precision of a double
					nItemLen = max(nItemLen, 312+nPrecision);
					break;

				case 'p':
					va_arg(argList, void*);
					nItemLen = 32;
					nItemLen = max(nItemLen, nWidth+nPrecision);
					break;

				case 'n':
					va_arg(argList, int*);
					break;

				default:
					printf("Warning in FormatPr. Unknown option %c in %s\n", *sz, szFormat);
			}
		}

		// adjust nMaxLen for output nItemLen
		nMaxLen += nItemLen;
	}

	char* szTmp = new char[nMaxLen + 1];
	vsprintf(szTmp, szFormat, argListSave);
	SetData(szTmp, 0);
	delete [] szTmp;

	va_end(argListSave);
}

//-----------------------------------------------------------------
// Private method used from Replace()
//-----------------------------------------------------------------
unsigned int CZorgeString::CountForReplace(const unsigned char* szData, unsigned int nDataLen,
										 const unsigned char* szSource, unsigned int nSourceLen,
										 const unsigned char* szTarget, unsigned int nTargetLen)
{
	unsigned int nResult(0);
	const unsigned char* szPtr = szData;
	unsigned int nLeft = nDataLen;
	while(*szPtr != 0)
	{
		if(nLeft < nSourceLen)
			break;
		if(strncmp((const char*)szPtr, (const char*)szSource, nSourceLen) == 0)
		{
			szPtr += nSourceLen;
			nLeft -= nSourceLen;
			++nResult;
		}
		else
		{
			++szPtr;
			--nLeft;
		}
	}
	return nResult;
}

//-----------------------------------------------------------------
// Replace in string data.
// Source - what to replace
// Target - replace with what
//-----------------------------------------------------------------
void CZorgeString::Replace(const char* szSource, const char* szTarget)
{
	if(*szSource == 0 || *szTarget == 0)
		return;

	if(m_szData == 0)
		return;

	unsigned int nSourceLen = (unsigned int)strlen(szSource);
	unsigned int nTargetLen = (unsigned int)strlen(szTarget);
	if(nSourceLen == 0 || nTargetLen == 0)
		return;

	unsigned char* szData = m_szData;
	unsigned int nDataLen = m_nUsed;
	if(nSourceLen == nTargetLen)
	{
		unsigned char* szPtr = szData;
		unsigned int nLeft = nDataLen;
		while(*szPtr != 0)
		{
			if(nLeft < nSourceLen)
				break;
			if(strncmp((const char*)szPtr, szSource, nSourceLen) == 0)
			{
				memcpy(szPtr, szTarget, nSourceLen);
				szPtr += nSourceLen;
				nLeft -= nSourceLen;
			}
			else
			{
				++szPtr;
				--nLeft;
			}
		}
		return;
	}
	int nNewSize = nDataLen + 1;
	if(nTargetLen > nSourceLen)
	{
		unsigned int nCount = CountForReplace(szData, nDataLen,
											  (const unsigned char*)szSource, nSourceLen,
											  (const unsigned char*)szTarget, nTargetLen);
		if(nCount == 0)
			return;

		nNewSize = nCount * (nTargetLen - nSourceLen) + nDataLen + 1;
	}

	char* szCopy = new char[nNewSize];

	// ready to copy
	char* szPtrDataLast = (char*)szData;
	char* szPtrData = (char*)szData;
	char* szPtrCopy = (char*)szCopy;
	unsigned int nLeft = nDataLen;
	while(*szPtrData != 0)
	{
		if(nLeft < nSourceLen)
			break;
		if(strncmp(szPtrData, szSource, nSourceLen) == 0)
		{
			unsigned int nUncopiedLen = (unsigned int)(szPtrData - szPtrDataLast);

			// copy skipped chars
			memcpy(szPtrCopy, szPtrDataLast, nUncopiedLen);
			szPtrCopy += nUncopiedLen;

			memcpy(szPtrCopy, szTarget, nTargetLen);
			szPtrData += nSourceLen;
			szPtrCopy += nTargetLen;
			szPtrDataLast = szPtrData;

			nLeft -= nSourceLen;
		}
		else
		{
			++szPtrData;
			--nLeft;
		}
	}

	unsigned int nLen = (unsigned int)strlen(szPtrDataLast);
	if(nLen != 0)
	{
		memcpy(szPtrCopy, szPtrDataLast, nLen);
		szPtrCopy += nLen;
	}
	*szPtrCopy = 0;
	SetData(szCopy, 0);
	delete [] szCopy;
}

//-----------------------------------------------------------------
// Replace in string data.
// Source - what to replace
// Target - replace with what
//-----------------------------------------------------------------
void CZorgeString::Replace(char chSource, char chTarget)
{
	if(chSource == 0 || chTarget == 0)
		return;

	if(m_szData == 0)
		return;

	unsigned char* szData = m_szData;
	for(; *szData != 0; ++szData)
	{
		if(*szData == chSource)
			*szData = chTarget;
	}
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
char CZorgeString::GetAt(unsigned int nPos) const
{
	if(m_nUsed == 0 || nPos > m_nUsed || m_szData == 0)
		return 0;

	return (char)m_szData[nPos];
}

//-----------------------------------------------------------------
// remove last char if exist
//-----------------------------------------------------------------
void CZorgeString::Chop()
{
	if(m_nUsed == 0 || m_szData == 0)
		return;

	m_nUsed--;
	m_szData[m_nUsed] = 0;
}

//-----------------------------------------------------------------
// remove first char if exist
//-----------------------------------------------------------------
void CZorgeString::ChopBegin()
{
	if(m_nUsed == 0 || m_szData == 0)
		return;

	unsigned char* szSource = m_szData + 1;
	unsigned char* szTarget = m_szData;
	for(; *szSource != 0; ++szSource, ++szTarget)
		*szTarget = *szSource;

	m_nUsed--;
	m_szData[m_nUsed] = 0;
}

//-----------------------------------------------------------------
// compare no case
//-----------------------------------------------------------------
int	CZorgeString::CompareNoCase(const char* szStr) const
{
	if(m_szData == 0 || szStr == 0)
		return -1;

    CZorgeString str1(szStr);
    CZorgeString str2(*this);

	str1.MakeUpper();
	str2.MakeUpper();

	unsigned char* szData1 = str1.m_szData;
	unsigned char* szData2 = str2.m_szData;
	return strcmp((const char*)szData1, (const char*)szData2);
}

//-----------------------------------------------------------------
// compare two strings
//-----------------------------------------------------------------
int	CZorgeString::Compare(const CZorgeString& str) const
{
	if(m_szData == 0 || str.m_szData == 0)
		return -1;

	return strcmp((char*)m_szData, (char*)str.m_szData);
}

//-----------------------------------------------------------------
// Allocate buffer
//-----------------------------------------------------------------
unsigned char* CZorgeString::GetBuffer(unsigned int nLen)
{
	if(m_nAlloc >= nLen)
		return m_szData; // I already have buffer big enough

	m_nAlloc = nLen;
	m_nUsed = 0;

	if(m_szData != 0)
		delete [] m_szData;

	m_szData = new unsigned char[m_nAlloc];
	m_szData[0] = 0;

	return m_szData;
}

//-----------------------------------------------------------------
// Init string data from buffer and length
// Pass nLen = 0 when copying whole string.
//-----------------------------------------------------------------
void CZorgeString::SetData(const char* szData, unsigned int nLen)
{
    if(szData == 0)
    {
    	m_nUsed = 0;
    	if(m_szData)
    		m_szData[0] = 0;
        return;
    }

	unsigned int nLenData = (unsigned int)strlen(szData);
	if(nLen == 0) // Caller passing 0 to use full string length.
		nLen = nLenData;
	unsigned int nLenToCopy = min(nLenData, nLen);
	if(nLenToCopy == 0)
	{
    	m_nUsed = 0;
    	if(m_szData)
    	{
    		m_szData[0] = 0;
    	}
		return;
	}

	if(nLenToCopy >= m_nAlloc)
	{
		// Re-allocate.
		unsigned int nAllocCalc = ((unsigned int)(nLen/16) + 1) * 16;
		if(nAllocCalc < m_nAllocStep)
			nAllocCalc = ((int)(m_nAllocStep / 16) + 1) * 16;
		m_nAlloc = nAllocCalc;

		if(m_szData != 0)
			delete [] m_szData;

		m_szData = new unsigned char[m_nAlloc];
	}

	m_nUsed = nLenToCopy;
	memcpy(m_szData, szData, nLenToCopy);
	m_szData[m_nUsed] = 0;
}

void _strupr(char* szStr)
{
	for(; *szStr != 0; ++szStr)
		*szStr = (unsigned char)toupper(*szStr);
}

//-----------------------------------------------------------------
// Split string into tokens
//-----------------------------------------------------------------
unsigned int CZorgeString::Split(char chSeparator, TTokens& vOut) const
{
    vOut.clear();

    CZorgeString strOne;
    unsigned char* pCur = m_szData;
    if(pCur == 0)
    	return 0;
    for(; *pCur != 0; ++pCur)
    {
        unsigned char ch = *pCur;
        if(ch == chSeparator)
        {
            vOut.push_back(strOne);
            strOne.Empty();
            continue;
        }
        strOne += ch;
    }
    if(!strOne.IsEmpty())
        vOut.push_back(strOne);

    return (unsigned int)vOut.size();
}

unsigned int CZorgeString::SplitPtr(char chSeparator, TTokensPtr& vOut) const
{
	if(vOut.size())
	{
		TTokensPtr::iterator it = vOut.begin();
		for(; it != vOut.end(); ++it)
			delete *it;
		vOut.clear();
	}

    CZorgeString strOne;
    unsigned char* pCur = m_szData;
    if(pCur == 0)
    	return 0;
    for(; *pCur != 0; ++pCur)
    {
        unsigned char ch = *pCur;
        if(ch == chSeparator)
        {
            vOut.push_back(new CZorgeString(strOne));
            strOne.Empty();
            continue;
        }
        strOne += ch;
    }
    if(!strOne.IsEmpty())
    	vOut.push_back(new CZorgeString(strOne));

    return (unsigned int)vOut.size();
}

/*
struct C12
{
	C12()
	{
		CZorgeString s1 = "123abc";
		CZorgeString s2;
		s2 = s1.Right(3);
		printf("s2=[%s]\n", (const char*)s2);
	}
} gC1;
*/
