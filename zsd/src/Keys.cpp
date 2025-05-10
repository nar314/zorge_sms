#include "Keys.h"
#include "StrUtils.h"
#include "SysUtils.h"

// I do not support pages of keys. 256 is more than enough at this point.
#define KEY_SIZE 256

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
CKeys::CKeys()
{
	m_nSize = KEY_SIZE;
	if(m_nSize < 2)
	{
		CZorgeString strErr;
		strErr.Format("Invalid key size value = %d\n", m_nSize);
		throw CZorgeAssert(strErr);
	}
	if(m_nSize > 255)
		m_nSize = 255;

	bool bDash = GetRandomByte() % 2 == 0;
	for(unsigned int i = 0; i < m_nSize; ++i)
	{
		CZorgeString* p = new CZorgeString();
		if(bDash)
			GuidStrDash(*p);
		else
			GuidStr(*p);

		m_vKeys.push_back(p);
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
CKeys::~CKeys()
{
	if(m_vKeys.size() == 0)
		return;

	for(TKeys::iterator it = m_vKeys.begin(); it != m_vKeys.end(); ++it)
		delete *it;
	m_vKeys.clear();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
unsigned char CKeys::GetNextKey(CZorgeString& strOut)
{
	if(m_vKeys.size() == 0)
		throw CZorgeAssert("m_vKeys.size() == 0");

	unsigned char ch = 0;
	for(unsigned int n = 0; n < 100; ++n)
	{
		ch = GetRandomByte();
		if(ch != 0 && ch < m_nSize)
			break;
	}

	if(ch == 0 || ch >= m_nSize)
		ch = 1;

	strOut = *m_vKeys[ch];
	return ch;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
const CZorgeString*	CKeys::GetKey(unsigned char nPos)
{
	size_t nIndex = nPos;
	if(nIndex >= m_vKeys.size())
		throw CZorgeError("KeyId is out of range.");
	return m_vKeys.at(nIndex);
}
