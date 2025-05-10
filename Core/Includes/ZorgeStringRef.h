// ZorgeStringRedf.h

#ifndef ZORGE_STRING_REF_H
#define ZORGE_STRING_REF_H

#include <vector>
using namespace std;

#include <string.h>
#include <stdio.h> // for va_list

#define STR_NOT_FOUND (-1)

//-------------------------------------------------
//
//-------------------------------------------------
struct CStrData
{
	unsigned char* 	m_szData;
	unsigned int 	m_nAlloc;
	unsigned int 	m_nUsed;
	unsigned int 	m_nRef;
	
					CStrData();
					CStrData(const CStrData&) = delete;
					CStrData operator=(const CStrData&) = delete;
	virtual 		~CStrData();

	void			SetData(const char*, unsigned int);
	void			SetData(const char*, unsigned int, unsigned int);
	void			AppendBinData(const char*, unsigned int, unsigned int);
	void			AppendData(const char*, unsigned int);
	CStrData*		Copy();
	void			Init(unsigned int, unsigned int);
	void			EmptyData();
};

class CZorgeString;
typedef vector<CZorgeString> TTokens;
typedef vector<CZorgeString*> TTokensPtr;

//-------------------------------------------------
//
//-------------------------------------------------
class CZorgeString
{
	CStrData*		m_pData;
	unsigned int	m_nAllocStep;
	
	void 			ReleaseData();
	void 			CopyData();
	void			FormatPr(const char*, va_list);
	unsigned int	CountForReplace(const unsigned char*, unsigned int, 
									const unsigned char*, unsigned int, 
									const unsigned char*, unsigned int);

public:
                    CZorgeString();
                    CZorgeString(const char*);
                    CZorgeString(const char*, unsigned int);
                    CZorgeString(const CZorgeString&);
	
    virtual 		~CZorgeString();
	
	unsigned int 	GetLength() const;
	unsigned int	GetAllocLength() const;
	void			Empty();
	void			EmptyData();
	bool			IsEmpty() const;
	void			SetAllocStep(unsigned int);
	void			SetData(const char*, unsigned int);
	void			AppendBinData(const char*, unsigned int);

	bool			IsFirstChar(char) const;
	bool			IsLastChar(char) const;
    char            GetLastChar() const;
	
	// assignments
    CZorgeString& 	operator=(const CZorgeString&);
    CZorgeString& 	operator+=(const CZorgeString&);
    CZorgeString& 	operator+=(const char*);
    CZorgeString& 	operator+=(const char);

	// comparison
	bool			operator==(const char*) const;
    bool			operator==(const CZorgeString&) const;
    bool			operator!=(const CZorgeString&) const;
	bool			operator!=(const char*) const;
	bool			operator<(const CZorgeString&) const;
	int				CompareNoCase(const char*) const;
    int				Compare(const CZorgeString&) const;

	operator		const char*() const;

	// trimming
	void			TrimRight();
	void			TrimLeft();
	void			Trim();
	void			Chop();
	void			ChopBegin();
	
    CZorgeString	Right(unsigned int) const;
    CZorgeString	Left(unsigned int) const;
    CZorgeString	Mid(unsigned int, unsigned int) const;

	// find
	int				Find(char) const;
	int				Find(const char*) const;	
	int				Find(char, unsigned int) const;
	int				Find(const char*, unsigned int) const;
	int				ReverseFind(char) const;
	
	void 			MakeUpper();
	void 			MakeLower();

	void			Replace(const char*, const char*);
	void			Replace(char, char);

	void			Format(const char*, ...);
	char			GetAt(unsigned int) const;
	unsigned char*	GetBuffer(unsigned int);

	void			Dump();
    unsigned int    Split(char, TTokens&) const;
    unsigned int    SplitPtr(char, TTokensPtr&) const;
};

// map<CZorgeString*, int, CMapCmpPtr> map;
struct CMapCmpPtr
{
    bool operator()(const CZorgeString* p1, const CZorgeString* p2) const
    {
    	if(p1 == 0)
    	{
    		puts("CMapCmpPtr. p1 == 0");
    		return true;
    	}
    	if(p2 == 0)
    	{
    		puts("CMapCmpPtr. p2 == 0");
    		return true;
    	}

        return *p1 < *p2;
    }
};

// map<CZorgeString, int, CMapCmp> map;
struct CMapCmp
{
    bool operator()(const CZorgeString& p1, const CZorgeString& p2) const
    {
        return p1 < p2;
    }
};

#endif

