// CoderRsa.h

#ifndef CORE_CODE_RSA_H_
#define CORE_CODE_RSA_H_

#include <openssl/types.h>
#include "MemBuffer.h"
#include "ZorgeMutex.h"

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
class CCoderRSA
{
	char*			m_szError;
	CZorgeMutex		m_Lock;
	CZorgeString	m_strPublicKey;

	BIGNUM* 		m_pBigNum;
	RSA*			m_pRSA;

	void	Clear();
	char*	GetError();
	void 	GeneratePublicKey_X509();

public:
	CCoderRSA(const CCoderRSA&) = delete;
	const CCoderRSA& operator=(const CCoderRSA&) = delete;

	CCoderRSA();
	~CCoderRSA();

	void	Init();
	void 	Encrypt(const unsigned char* szInput, int nInputLen, CMemBuffer& Out);
	void	Decrypt(const CMemBuffer& In, CMemBuffer& Out);

	const CZorgeString&	GetPublicKey() const;
};

#endif
