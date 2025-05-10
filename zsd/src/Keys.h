// Keys.h

#ifndef C_KEYS_H_
#define C_KEYS_H_

#include <vector>
#include "ZorgeString.h"

typedef vector<CZorgeString*> TKeys;
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
class CKeys
{
	unsigned int	m_nSize;
	TKeys			m_vKeys;

public:
	CKeys(const CKeys&) = delete;
	CKeys& operator=(const CKeys&) = delete;

	CKeys();
	~CKeys();
	unsigned char GetNextKey(CZorgeString& strOut);
	const CZorgeString*	GetKey(unsigned char nPos);
};

#endif
