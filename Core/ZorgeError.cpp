#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "ZorgeError.h"

//----------------------------------------------------------
// signal and memory allocation handler
//----------------------------------------------------------
#ifdef WIN32
    #include <eh.h>
    #include <new.h> // for _set_new_handler

    void SignalHandler(unsigned int nCode, _EXCEPTION_POINTERS*)
    {
        CZorgeString str;
        str.Format("Unhandled signal ! code %X", nCode);
        throw CZorgeSignalError(nCode, str);
    }

    int MemoryHandler(size_t)
    {
        puts("Zorge memory handlder. NO MORE MEMORY !");
        exit(-911);
    }
#else
    #include <new> // for set_new_handler
    void MemoryHandler()
    {
        puts("Memory handler. NO MORE MEMORY !");
        exit(-911);
    }

    void SignalHandler(int nSignal)
    {
        const char* szSigName = 0;
        switch(nSignal)
        {
            case SIGINT:    szSigName = "SIGINT"; break;
            case SIGILL:    szSigName = "SIGILL"; break;
            case SIGABRT:   szSigName = "SIGABRT"; break;
            case SIGFPE:    szSigName = "SIGFPE"; break;
            case SIGSEGV:   szSigName = "SIGSEGV"; break;
            case SIGTERM:   szSigName = "SIGTERM"; break;
            case SIGHUP:    szSigName = "SIGHUP"; break;
            case SIGQUIT:   szSigName = "SIGQUIT"; break;
            case SIGTRAP:   szSigName = "SIGTRAP"; break;
            case SIGKILL:   szSigName = "SIGKILL"; break;
            case SIGPIPE:   szSigName = "SIGPIPE"; break;
            case SIGALRM:   szSigName = "SIGALRM"; break;
            default: szSigName = "";
        }
        // Will I have memory and stack for printf ?
        printf("\n\n------------- signal handler, nSignal = %d, %s\nexit()\n", nSignal, szSigName);
        exit(907);
    }

#endif

CSetSignalHandler::CSetSignalHandler()
{
#ifdef WIN32
	_set_se_translator(SignalHandler);
    _set_new_handler(MemoryHandler);
#else
    std::set_new_handler(&MemoryHandler);

    //signal(SIGTERM, SignalHandler);
/* do coredump for those.
    signal(SIGSEGV, SignalHandler);
    signal(SIGABRT, SignalHandler);

    signal(SIGILL, SignalHandler);
    signal(SIGINT, SignalHandler);
    signal(SIGFPE, SignalHandler);
*/
#endif
}

//----------------------------------------------------------
//
//----------------------------------------------------------
CZorgeError::CZorgeError(const char* szMsg)
{
	m_strMsg = szMsg;
}

CZorgeError::CZorgeError(const char* szMsg, const char* szName, int nLine)
{
	m_strMsg.Format("%s, [%s:%d]", szMsg, szName ? szName : "", nLine);
}

CZorgeError::CZorgeError(const CZorgeError& copy)
{
	m_strMsg = copy.m_strMsg;
}

CZorgeError::~CZorgeError()
{
}

const CZorgeString& CZorgeError::GetMsg() const
{
	return m_strMsg;
}

const CZorgeString& CZorgeError::GetMsg()
{
	return m_strMsg;
}

const char* CZorgeError::what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW
{
	return (const char*)m_strMsg;
}

//----------------------------------------------------------
//
//----------------------------------------------------------
CZorgeSignalError::CZorgeSignalError(unsigned int nCode, const char* szMsg)
: CZorgeError(szMsg)
{
	m_nCode = nCode;
}

CZorgeSignalError::CZorgeSignalError(const CZorgeSignalError& copy)
: CZorgeError(copy.GetMsg())
{
	m_nCode = copy.m_nCode;
}

CZorgeSignalError::~CZorgeSignalError()
{
}

unsigned int CZorgeSignalError::GetCode()
{
	return m_nCode;
}
