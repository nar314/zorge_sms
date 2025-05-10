// CoderBF.h
#ifndef CORE_CODERBF_H_
#define CORE_CODERBF_H_
#include <openssl/blowfish.h>

#include "MemBuffer.h"

#define BF_IV_SIZE 8

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
class CCoderBF
{
	BF_KEY			m_Key;
	unsigned char 	m_IV[BF_IV_SIZE];

	bool CalcMD5(unsigned char*, size_t, CMemBuffer&);
	void	Encrypt(unsigned char*, size_t, CMemBuffer&);
	void 	Decrypt(unsigned char*, size_t, CMemBuffer&);

public:
	CCoderBF(const CCoderBF&) = delete;
	CCoderBF& operator=(const CCoderBF&) = delete;

	CCoderBF();
	~CCoderBF();

	void	Init(const CZorgeString&);
	void 	EncryptToHexChars(const CZorgeString&, CZorgeString&);
	void 	DecryptFromHexChars(const CZorgeString&, CZorgeString&);

};

#endif
