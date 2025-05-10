// MemBuffer.h

#ifndef CORE_MEMBUFFER_H_
#define CORE_MEMBUFFER_H_

#include <cstddef> // for size_t
#include "ZorgeString.h"

//----------------------------------------------------------
//
//----------------------------------------------------------
class CMemBuffer
{
	unsigned char* 	m_pBuffer;
	size_t			m_nSizeAllocated;
	size_t			m_nSizeData;

public:
	CMemBuffer(size_t = 0);
	CMemBuffer(const CZorgeString&);
	CMemBuffer(unsigned char* szData, size_t nDataSize);

	CMemBuffer(const CMemBuffer&) = delete;
	CMemBuffer& operator=(const CMemBuffer&) = delete;

	~CMemBuffer();

	void			ToString(CZorgeString& strOut) const;
	size_t			GetSizeData() const;
	unsigned char*	Get() const;
	size_t			GetSizeAllocated() const;
	bool			SetByte(size_t nIndex, unsigned char chByte);

	unsigned char*	Init(unsigned char* szData, size_t nDataSize);
	int				InitFromHexChars(const CZorgeString& strInput);
	bool			SetSizeData(size_t nSizeData);
	unsigned char*	Allocate(size_t nRequestedSize);
	void			Release();
	bool			IsEqual(const CMemBuffer& Other) const;

	void			Print();
};

#endif
