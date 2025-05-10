#include <string.h> // memcpy
#include <stdio.h> // for printf
#include "MemBuffer.h"
#include "StrUtils.h"
#include <atomic>

std::atomic<unsigned int> g_CMemBuffer = 0;
std::atomic<unsigned int> g_CMemBuffer_max = 0;

void DiagCMemBuffer(CZorgeString& strOut)
{
	CZorgeString str;
	str.Format("CMemBuffer.\tCur=%d, Max=%d\n", (unsigned int)g_CMemBuffer, (unsigned int)g_CMemBuffer_max);
	strOut += str;
}

struct CMemBufferCounter
{
	~CMemBufferCounter()
	{
		printf("CMemBuffer = %d (%d)\n", (unsigned int)g_CMemBuffer, (unsigned int)g_CMemBuffer_max);
	}
} g_CMemBufferCounter;

//----------------------------------------------------------
//
//----------------------------------------------------------
CMemBuffer::CMemBuffer(size_t nSize)
{
	g_CMemBuffer++;
	g_CMemBuffer_max++;
	m_nSizeData = 0;
	if(nSize)
	{
		m_pBuffer = new unsigned char[nSize];
		m_nSizeAllocated = nSize;
		//memset(m_pBuffer, 0, nSize);
	}
	else
	{
		m_pBuffer = 0;
		m_nSizeAllocated = 0;
	}
}

//----------------------------------------------------------
//
//----------------------------------------------------------
CMemBuffer::CMemBuffer(const CZorgeString& strInput)
{
	g_CMemBuffer++;
	g_CMemBuffer_max++;
	m_nSizeData = 0;
	unsigned int nSize = strInput.GetLength();
	if(nSize)
	{
		m_pBuffer = new unsigned char[nSize]; // No place for null termination !
		m_nSizeAllocated = nSize;
		m_nSizeData = nSize;
		memcpy(m_pBuffer, (const char*)strInput, nSize);
	}
	else
	{
		m_pBuffer = 0;
		m_nSizeAllocated = 0;
	}
}

//----------------------------------------------------------
//
//----------------------------------------------------------
CMemBuffer::CMemBuffer(unsigned char* szData, size_t nDataSize)
{
	g_CMemBuffer++;
	g_CMemBuffer_max++;
	m_pBuffer = 0;
	m_nSizeAllocated = m_nSizeData = 0;

	Init(szData, nDataSize);
}

//----------------------------------------------------------
//
//----------------------------------------------------------
CMemBuffer::~CMemBuffer()
{
	g_CMemBuffer--;
	if(m_pBuffer != 0)
		delete [] m_pBuffer;
}

//----------------------------------------------------------
//
//----------------------------------------------------------
unsigned char* CMemBuffer::Init(unsigned char* szData, size_t nDataSize)
{
	memcpy(Allocate(nDataSize), szData, nDataSize);
	m_nSizeData = nDataSize;
	return m_pBuffer;
}

//----------------------------------------------------------
//
//----------------------------------------------------------
int	CMemBuffer::InitFromHexChars(const CZorgeString& strInput)
{
	unsigned int nInputLen = strInput.GetLength();
	if(nInputLen % 2 != 0)
		return 1; // Bad input

	int nPairs = nInputLen / 2;

	Allocate(nPairs);

	const char* szInput = (const char*)strInput;
	unsigned char* szBuf = Get();
	char chLeft(0), chRight(0);
	for(int i = 0; i < nPairs; ++i)
	{
		chLeft = *szInput++;
		chRight = *szInput++;
		unsigned char ch = HexCharsToByte(chLeft, chRight);
		*szBuf++ = ch;
	}
	SetSizeData(nPairs);
	return 0; // OK
}

//----------------------------------------------------------
//
//----------------------------------------------------------
unsigned char* CMemBuffer::Allocate(size_t nRequestedSize)
{
	if(nRequestedSize <= m_nSizeAllocated)
		return m_pBuffer;

	if(m_pBuffer != 0)
		delete [] m_pBuffer;

	m_pBuffer = new unsigned char[nRequestedSize];
	m_nSizeAllocated = nRequestedSize;
	m_nSizeData = 0;
	return m_pBuffer;
}

//----------------------------------------------------------
//
//----------------------------------------------------------
unsigned char* CMemBuffer::Get() const
{
	return m_pBuffer;
}

//----------------------------------------------------------
//
//----------------------------------------------------------
size_t CMemBuffer::GetSizeAllocated() const
{
	return m_nSizeAllocated;
}

//----------------------------------------------------------
//
//----------------------------------------------------------
void CMemBuffer::Release()
{
	if(m_pBuffer != 0)
	{
		delete [] m_pBuffer;
		m_pBuffer = 0;
		m_nSizeAllocated = 0;
	}
}

//----------------------------------------------------------
//
//----------------------------------------------------------
bool CMemBuffer::IsEqual(const CMemBuffer& Other) const
{
	if(Other.m_nSizeData != m_nSizeData)
		return false;

	return memcmp(m_pBuffer, Other.m_pBuffer, m_nSizeData) == 0;
}

//----------------------------------------------------------
// return true if success
//----------------------------------------------------------
bool CMemBuffer::SetSizeData(size_t nSizeData)
{
	if(nSizeData <= m_nSizeAllocated)
	{
		m_nSizeData = nSizeData;
		return true;
	}
	return false;
}

//----------------------------------------------------------
//
//----------------------------------------------------------
size_t CMemBuffer::GetSizeData() const
{
	return m_nSizeData;
}

//----------------------------------------------------------
// return true if success
//----------------------------------------------------------
bool CMemBuffer::SetByte(size_t nIndex, unsigned char chByte)
{
	if(nIndex >= m_nSizeAllocated)
		return false;

	m_pBuffer[nIndex] = chByte;
	return true;
}

void CMemBuffer::Print()
{
	printf("aloc=%ld, data=%ld\n", m_nSizeAllocated, m_nSizeData);
	if(m_pBuffer == 0)
		return;
	int j = 0;
	size_t i = 0;
	for(i = 0; i < m_nSizeAllocated; ++i, ++j)
	{
		if(j == 32)
		{
			puts(""); // 32 bytes per line
			j = 0;
		}
		printf("%02X ", m_pBuffer[i]);
	}
	puts("");
}

//----------------------------------------------------------
//
//----------------------------------------------------------
void CMemBuffer::ToString(CZorgeString& strOut) const
{
	if(m_nSizeData == 0)
	{
		strOut.EmptyData();
		return;
	}
	strOut.SetData((const char*)m_pBuffer, m_nSizeData);
}

/*
struct CV1
{
	CV1()
	{
		CMemBuffer b;
		strcpy((char*)b.Allocate(5), "12345");
		if(!b.SetByte(4, 0xFF))
			puts("not set.");
		b.Print();
		return;


		CMemBuffer b2;
		strcpy((char*)b2.Allocate(64), "Hello world1");

		bool bz = b.IsEqual(b2);

		printf("%d\n", bz);
	}
} za;
*/
