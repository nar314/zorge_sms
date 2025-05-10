#include "Config.h"
#include <atomic>

#define MAX_NAME_LEN	128
#define MAX_VALUE_LEN 	1024

CConfig g_Config;

std::atomic<unsigned int> g_CConfig = 0;
std::atomic<unsigned int> g_CConfig_max = 0;

void DiagCConfig(CZorgeString& strOut)
{
	CZorgeString str;
	str.Format("CConfig.\tCur=%d, Max=%d\n", (unsigned int)g_CConfig, (unsigned int)g_CConfig_max);
	strOut += str;
}

struct CConfigCounter
{
	~CConfigCounter()
	{
		printf("CConfig = %d (%d)\n", (unsigned int)g_CConfig, (unsigned int)g_CConfig_max);
	}
} g_CConfigCounter;

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
CConfig::CConfig()
{
	g_CConfig++;
	g_CConfig_max++;
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
CConfig::~CConfig()
{
	g_CConfig--;
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
void CConfig::Clear()
{
	m_mComments.clear();
	m_mValues.clear();
}

static bool SplitPair(const CZorgeString& strLine, CZorgeString& strName, CZorgeString& strValue)
{
	const char* szLine = (const char*)strLine;

	unsigned int nLen = strLine.GetLength();
	unsigned int n = 0;
	for(; *szLine != 0; ++n, ++szLine)
		if(*szLine == '=')
			break;

	if(n == nLen || n + 1 == nLen || n == 0) // '=' not found or '=' found as last char or '=' is a first char
		return false;

	szLine = (const char*)strLine;
	strName.SetData(szLine, n);
	strValue.SetData((szLine + n + 1), nLen - n - 1);
	return strName.GetLength() > 0 && strValue.GetLength() > 0;
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
int CConfig::FromString(const CZorgeString& strData)
{
	Clear();

	TTokens vTokens;
	if(strData.Split('\n', vTokens) == 0)
		return 0;

	unsigned int nLine = 0;
	CZorgeString strComments, strName, strValue;
	TTokens::const_iterator it = vTokens.begin();
	for(; it != vTokens.end(); ++it, ++nLine)
	{
		const CZorgeString& strLine = *it;
		if(strLine.IsEmpty())
			strComments += "\n";
		else if(strLine.IsFirstChar('#'))
		{
			strComments += strLine;
			strComments += "\n";
		}
		else
		{
			if(!SplitPair(strLine, strName, strValue))
			{
				// Split fail. Consider as a comment line
				strComments += strLine;
				strComments += "\n";
			}
			else
			{
				strName.Trim(); strValue.Trim();
				if(strName.GetLength() > MAX_NAME_LEN || strValue.GetLength() > MAX_VALUE_LEN)
				{
					printf("Config name or value is out of range. name len = %d, value len = %d\n", strName.GetLength(), strValue.GetLength());
					strComments.EmptyData();
					continue;
				}

				m_mValues.push_back(make_pair(strName, strValue));
				if(!strComments.IsEmpty())
					m_mComments.insert(make_pair(strName, strComments));
				strComments.EmptyData();
			}
		}
	}
	return m_mValues.size();
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
void CConfig::Print()
{
	printf("m_mValues.size() = %ld, m_mComments.size() = %ld\n", m_mValues.size(), m_mComments.size());

	puts("Values:");
	TValues::const_iterator it = m_mValues.begin();
	for(; it != m_mValues.end(); ++it)
		printf("[%s] = [%s]\n", (const char*)(*it).first, (const char*)(*it).second);

	puts("Comments:");
	TMap::const_iterator itV = m_mComments.begin();
	for(; itV != m_mComments.end(); ++itV)
		printf("[%s] = [%s]\n", (const char*)(*itV).first, (const char*)(*itV).second);

	puts("");
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
void CConfig::ToString(CZorgeString& strOut)
{
	strOut.EmptyData();
	TValues::const_iterator itV;
	for(itV = m_mValues.begin(); itV != m_mValues.end(); ++itV)
	{
		const CZorgeString& strName = (*itV).first;
		const CZorgeString& strValue = (*itV).second;

		TMap::const_iterator itC = m_mComments.find(strName);
		if(itC != m_mComments.end())
			strOut += (*itC).second;

		strOut += strName; strOut += '='; strOut += strValue; strOut += "\n";
		strOut += "\n"; // empty line between parameters.
	}
}

//-----------------------------------------------------------------
// True - found
// False - not found
//-----------------------------------------------------------------
bool CConfig::GetValue(CZorgeString& strName, CZorgeString& strOut) const
{
	return GetValue((const char*)strName, strOut);
}

//-----------------------------------------------------------------
// True - found
// False - not found
//-----------------------------------------------------------------
bool CConfig::GetValue(const char* szName, CZorgeString& strOut) const
{
	TValues::const_iterator itV = m_mValues.begin();
	for(; itV != m_mValues.end(); ++itV)
	{
		const CZorgeString& str = (*itV).first;
		if(str == szName)
		{
			strOut = (*itV).second;
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------
// True - success
// False - failure
//-----------------------------------------------------------------
bool CConfig::SetValue(CZorgeString& strName, CZorgeString& strNewValue)
{
	strName.Trim();
	strNewValue.Trim();

	return SetValue((const char*)strName, strNewValue);
}

//-----------------------------------------------------------------
// True - success
// False - failure
//-----------------------------------------------------------------
bool CConfig::SetValue(const char* szName, CZorgeString& strNewValue)
{
	strNewValue.Trim();

	if(strlen(szName) > MAX_NAME_LEN || strNewValue.GetLength() > MAX_VALUE_LEN)
	{
		printf("Config name or value is out of range. name len = %ld, value len = %d\n", strlen(szName), strNewValue.GetLength());
		return false;
	}

	TValues::iterator itV = m_mValues.begin();
	for(; itV != m_mValues.end(); ++itV)
	{
		const CZorgeString& str = (*itV).first;
		if(str == szName)
		{
			(*itV).second = strNewValue;
			return true;
		}
	}

	m_mValues.push_back(make_pair(CZorgeString(szName), strNewValue));
	return true;
}

//-----------------------------------------------------------------
// True - success
// False - failure
//-----------------------------------------------------------------
bool CConfig::SetComment(const char* szName, const char* szComment)
{
	if(szName == 0)
	{
		puts("Config parameter name is null.");
		return false;
	}

	if(strlen(szName) > MAX_NAME_LEN)
	{
		printf("Config name is too long. name len = %ld\n", strlen(szName));
		return false;
	}

	CZorgeString strTmp;
	if(!GetValue(szName, strTmp))
	{
		printf("Value [%s] not found for comment [%s]\n", szName, szComment);
		return false;
	}

	CZorgeString strName(szName);
	TMap::iterator itC = m_mComments.find(strName);
	if(itC != m_mComments.end())
	{
		CZorgeString& strComments = (*itC).second;
		strComments += "# ";
		strComments += szComment;
		strComments += "\n";
	}
	else
	{
		strTmp = "# ";
		strTmp += szComment;
		strTmp += "\n";
		m_mComments.insert(make_pair(strName, strTmp));
	}
	return true;
}

CConfig& GetConfig()
{
	return g_Config;
}

/*
struct C1
{
	void F1()
	{
		CConfig c;
		CZorgeString s, v, o;

		s = "Param1"; v = "Value1";
		c.SetValue(s, v);

		s = "Param2"; v = "Value2";
		c.SetValue(s, v);

		c.ToString(o);
		puts("----------------------------");
		printf("%s\n", (const char*)o);
		puts("----------------------------");

		s = "Param2"; v = "Comment 1 for param2";
		c.SetComment(s, v);
		v = "Comment 2 for param2";
		c.SetComment(s, v);

		s = "Param1"; v = "Comment 1 for param1";
		c.SetComment(s, v);
		c.ToString(o);
		puts("----------------------------");
		printf("%s\n", (const char*)o);
		puts("----------------------------");

	}

	C1()
	{
		F1();
		return;

		CZorgeString strData, strOut;
		strData = "#This is config file for repository\n";
		strData += "#Repository id.\n";
		strData += "RepoId=111-222-333-444-555\n";
		strData += "\n";
		strData += "#Port.\n";
		strData += "Port=7530.\n";

		printf("[%s]\n", (const char*)strData);

		CConfig c;
		int n = c.FromString(strData);
		printf("Load return %d\n", n);
		c.Print();

		CZorgeString strValue("16");
		c.SetValue("Port", strValue);

		c.ToString(strOut);
		printf("[%s]\n", (const char*)strOut);
	}
} z11;
*/
