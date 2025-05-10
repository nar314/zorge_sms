// CoderAES.h

#ifndef CORE_CODERAES_H_
#define CORE_CODERAES_H_

#include <openssl/types.h>
#include "MemBuffer.h"
#include "ZorgeString.h"

#define CODER_OK 	1
#define CODER_ERROR 2

#define KEY_SIZE	32
#define IV_SIZE 	16

#define COM_ENCODER true
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
class CCoderAES
{
	bool			m_bCom;
	CZorgeString	m_strKeyIV;
	CMemBuffer		m_Key{KEY_SIZE};	// 32 bytes long
	CMemBuffer		m_IV{IV_SIZE};		// 16 bytes long

	EVP_CIPHER_CTX* m_pCtxEncr;
	EVP_CIPHER_CTX* m_pCtxDecr;

	EVP_MD_CTX* 	m_pCtxMd5;

	CZorgeString	m_strKey;

	bool 	CalcMD5(unsigned char*, size_t, CMemBuffer&);
	int		Init(unsigned char*, size_t, int);

	int		Encrypt(unsigned char*, size_t, CMemBuffer&);
	int		Decrypt(unsigned char*, size_t, CMemBuffer&);
public:
	CCoderAES(const CCoderAES&) = delete;
	CCoderAES& operator=(const CCoderAES&) = delete;

	CCoderAES(bool bCom = false);
	~CCoderAES();

	void	Init(const CZorgeString& strPsw);
	int 	EncryptToHexChars(const CZorgeString&, CZorgeString&);
	int 	DecryptFromHexChars(const CZorgeString&, CZorgeString&);

	int		Encrypt(const CMemBuffer&, CMemBuffer&);
	int		Decrypt(const CMemBuffer&, CMemBuffer&);
};

#endif

