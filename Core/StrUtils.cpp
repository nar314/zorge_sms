#include <stdint.h>
#include <stdlib.h>

#include "StrUtils.h"

static uint32_t rand32()
{
    return ((rand() & 0x3) << 30) | ((rand() & 0x7fff) << 15) | (rand() & 0x7fff);
}

#define GUID_DASHES_LEN 	37 // 36 + 1 -> 20f362d9-07c6-40e1-83a0-6d65f97e347d
#define GUID_NO_DASHES_LEN 	33 // 32 + 1 -> c831143fc2db4a61a14b170f8174dd7

//---------------------------------------------------------
//
//---------------------------------------------------------
bool GuidStrDash(CZorgeString& strOut)
{
	char szBuf[GUID_DASHES_LEN];
	int nLen = GUID_DASHES_LEN;
	int n = snprintf(szBuf, nLen, "%08X-%04X-%04X-%04X-%04X%08X",
	        rand32(),                         // Generates a 32-bit Hex number
	        rand32() & 0xffff,                // Generates a 16-bit Hex number
	        ((rand32() & 0x0fff) | 0x4000),   // Generates a 16-bit Hex number of the form 4xxx (4 indicates the UUID version)
	        (rand32() & 0x3fff) + 0x8000,     // Generates a 16-bit Hex number in the range [0x8000, 0xbfff]
	        rand32() & 0xffff, rand32());     // Generates a 48-bit Hex number

	bool bOK = n == GUID_DASHES_LEN - 1;
	if(!bOK)
		strOut.EmptyData();
	else
		strOut = szBuf;

	return bOK;
}

//---------------------------------------------------------
//
//---------------------------------------------------------
bool GuidStr(CZorgeString& strOut)
{
	char szBuf[GUID_NO_DASHES_LEN];
	int nLen = GUID_NO_DASHES_LEN;
	int n = snprintf(szBuf, nLen, "%08X%04X%04X%04X%04X%08X",
	        rand32(),                         // Generates a 32-bit Hex number
	        rand32() & 0xffff,                // Generates a 16-bit Hex number
	        ((rand32() & 0x0fff) | 0x4000),   // Generates a 16-bit Hex number of the form 4xxx (4 indicates the UUID version)
	        (rand32() & 0x3fff) + 0x8000,     // Generates a 16-bit Hex number in the range [0x8000, 0xbfff]
	        rand32() & 0xffff, rand32());     // Generates a 48-bit Hex number

	bool bOK = n == GUID_NO_DASHES_LEN - 1;
	if(!bOK)
		strOut.EmptyData();
	else
		strOut = szBuf;

	return bOK;
}

//---------------------------------------------------------
//
//---------------------------------------------------------
void strcpyz(char* szTarget, unsigned int nTargetLen, const char* szSource)
{
	const char* chSource = szSource;
	char* chTarget = szTarget;
	unsigned int nLeft = nTargetLen;
	for(; *chSource != 0; ++chSource, ++chTarget)
	{
		if(--nLeft == 0)
		{
			szTarget[nTargetLen] = 0;
			break;
		}
		*chTarget = *chSource;
	}
	*chTarget = 0;

	// Fill with nulls rest of the buffer.
	for(; nLeft; nLeft--, ++chTarget)
		*chTarget = 0;
}

//---------------------------------------------------------
//
//---------------------------------------------------------
static char g_szHexStr[256][3];
void InitHexStr()
{
	char szOne[3];
	for(int i = 0; i < 256; ++i)
	{
		sprintf(szOne, "%02X", i);
		g_szHexStr[i][0] = szOne[0];
		g_szHexStr[i][1] = szOne[1];
		g_szHexStr[i][2] = 0;
	}
}

struct CStrUtilsRunOnce
{
	CStrUtilsRunOnce()
	{
		InitHexStr();
	}

} g_StrUtilsRunOnce;

const char* ByteToHexStr(unsigned char ch)
{
	return g_szHexStr[ch];
}

unsigned char HexCharsToByte(char chLeft, char chRight)
{
	unsigned char chOut = 0x00;
	unsigned char ch = 0x00;

	if(chLeft >= '0' && chLeft <= '9')
		ch = chLeft - '0';
	else if(chLeft >= 'A' && chLeft <= 'F')
		ch = chLeft - 'A' + 10;
	else
		return 0; // Error !

	chOut = ch << 4;

	if(chRight >= '0' && chRight <= '9')
		ch = chRight - '0';
	else if(chRight >= 'A' && chRight <= 'F')
		ch = chRight - 'A' + 10;
	else
		return 0; // Error !

	chOut = chOut | ch;
	return chOut;
}

/*
struct CX2
{
	CX2()
	{
		InitHexStr();
		unsigned char chLeft(0), chRight(0);
		char szTmp[16];

		unsigned long nCount = 0;
		for(; true; ++nCount)
		{
			if(nCount % 10000000 == 0)
				printf("nCount = %ld M\n", nCount / 10000000);
			unsigned char ch = (unsigned char)rand32();
			sprintf(szTmp, "%02X", ch);
			chLeft = szTmp[0];
			chRight = szTmp[1];
			unsigned char chRes = HexCharsToNum(chLeft, chRight);

			if(chRes != ch)
			{
				printf("%02X -> %c%c => %02X\n", ch, chLeft, chRight, chRes);
				break;
			}
		}
	}
} za;
*/
