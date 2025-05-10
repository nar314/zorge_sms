// ZorgeError.h

#ifndef C_ZORGE_ERROR
#define C_ZORGE_ERROR

#include "ZorgeString.h"

#define ALL_ERRORS exception

#define CZorgeAssert(s) CZorgeError(s, __FILE__, __LINE__)
//#define CZorgeAssert(s) CZorgeError(s, __FUNCTION__, __LINE__)

//----------------------------------------------------------
//
//----------------------------------------------------------
class CZorgeError : public exception
{
    CZorgeString m_strMsg;

public :
    CZorgeError& operator= (const CZorgeError&) = delete;

    CZorgeError(const char*, const char*, int);
    CZorgeError(const char*);
    CZorgeError(const CZorgeError&);

    virtual ~CZorgeError();
    const CZorgeString&   GetMsg() const;
    const CZorgeString&   GetMsg();

    virtual const char* what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW;
};

//----------------------------------------------------------
//
//----------------------------------------------------------
class CZorgeSignalError : public CZorgeError
{
	unsigned int	m_nCode;
public:
                    CZorgeSignalError(unsigned int, const char*);
                    CZorgeSignalError(const CZorgeSignalError&);
    virtual			~CZorgeSignalError();
	unsigned int	GetCode();
};

//----------------------------------------------------------
//
//----------------------------------------------------------
struct CSetSignalHandler
{
    CSetSignalHandler();
};

#endif
