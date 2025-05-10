#include "CoderAES.h"
#include <atomic>

#include <dlfcn.h>
// Did you do
// sudo apt-get install libssl-dev
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/md5.h>

#include "StrUtils.h"
#include "ZorgeError.h"

typedef EVP_MD_CTX* (*T_EVP_MD_CTX_new)(void);
typedef const EVP_MD* (*T_EVP_md5)(void);
typedef void (*T_EVP_MD_CTX_free)(EVP_MD_CTX*);
typedef int (*T_EVP_DigestInit_ex2)(EVP_MD_CTX*, const EVP_MD*, const OSSL_PARAM []);
typedef int (*T_EVP_DigestUpdate)(EVP_MD_CTX*, const void*, size_t);
typedef int (*T_EVP_DigestFinal_ex)(EVP_MD_CTX*, unsigned char*, unsigned int*);

typedef const EVP_CIPHER* (*T_EVP_aes_256_cbc)(void);
typedef void (*T_EVP_CIPHER_CTX_free)(EVP_CIPHER_CTX*);
typedef EVP_CIPHER_CTX* (*T_EVP_CIPHER_CTX_new)(void);

typedef int (*T_EVP_EncryptInit_ex)(EVP_CIPHER_CTX*, const EVP_CIPHER*, ENGINE*, const unsigned char*, const unsigned char*);
typedef int (*T_EVP_EncryptUpdate)(EVP_CIPHER_CTX*, unsigned char*, int*, const unsigned char*, int);
typedef int (*T_EVP_EncryptFinal_ex)(EVP_CIPHER_CTX*, unsigned char*, int*);

typedef int (*T_EVP_DecryptInit_ex)(EVP_CIPHER_CTX*, const EVP_CIPHER*, ENGINE*, const unsigned char*, const unsigned char*);
typedef int (*T_EVP_DecryptUpdate)(EVP_CIPHER_CTX*, unsigned char*, int*, const unsigned char*, int);
typedef int (*T_EVP_DecryptFinal_ex)(EVP_CIPHER_CTX*, unsigned char*, int*);

// MD5
static T_EVP_MD_CTX_new g_EVP_MD_CTX_new = 0;
static T_EVP_md5 g_EVP_md5 = 0;
static T_EVP_MD_CTX_free g_EVP_MD_CTX_free = 0;
static T_EVP_DigestInit_ex2 g_EVP_DigestInit_ex2 = 0;
static T_EVP_DigestUpdate g_EVP_DigestUpdate = 0;
static T_EVP_DigestFinal_ex g_EVP_DigestFinal_ex = 0;

static T_EVP_aes_256_cbc g_EVP_aes_256_cbc = 0;
static T_EVP_CIPHER_CTX_free g_EVP_CIPHER_CTX_free = 0;
static T_EVP_CIPHER_CTX_new g_EVP_CIPHER_CTX_new = 0;

// Encrypt
static T_EVP_EncryptInit_ex g_EVP_EncryptInit_ex = 0;
static T_EVP_EncryptUpdate g_EVP_EncryptUpdate = 0;
static T_EVP_EncryptFinal_ex g_EVP_EncryptFinal_ex = 0;

// Decrypt
static T_EVP_DecryptInit_ex g_EVP_DecryptInit_ex = 0;
static T_EVP_DecryptUpdate g_EVP_DecryptUpdate = 0;
static T_EVP_DecryptFinal_ex g_EVP_DecryptFinal_ex = 0;

#define OPER_ENCR	0
#define OPER_DECR	1

static void* g_pOpenSSLHandle = 0;

//#ifdef __linux__
#define LIB_CRYPTO_NAME "libcrypto.so"
//#endif

#define CHECK_FN_PTR(p, n) if(!p) { CZorgeString s; s.Format("Failed to load openSSL function '%s'", n); throw CZorgeError(s);}
static void LoadOpenSSL_AES()
{
    if(g_pOpenSSLHandle)
        return;

    g_pOpenSSLHandle = dlopen(LIB_CRYPTO_NAME, RTLD_NOW); // I never close this handle.
    if(g_pOpenSSLHandle == 0)
    {
		CZorgeString strMsg;
		strMsg.Format("Failed to load openSSL library '%s'", LIB_CRYPTO_NAME);
		throw CZorgeError(strMsg);
    }

    g_EVP_MD_CTX_new = (T_EVP_MD_CTX_new)dlsym(g_pOpenSSLHandle, "EVP_MD_CTX_new");
    CHECK_FN_PTR(g_EVP_MD_CTX_new, "EVP_MD_CTX_new");

    g_EVP_md5 = (T_EVP_md5)dlsym(g_pOpenSSLHandle, "EVP_md5");
    CHECK_FN_PTR(g_EVP_md5, "EVP_md5");

    g_EVP_MD_CTX_free = (T_EVP_MD_CTX_free)dlsym(g_pOpenSSLHandle, "EVP_MD_CTX_free");
    CHECK_FN_PTR(g_EVP_MD_CTX_free, "EVP_MD_CTX_free");

    g_EVP_DigestInit_ex2 = (T_EVP_DigestInit_ex2)dlsym(g_pOpenSSLHandle, "EVP_DigestInit_ex2");
    CHECK_FN_PTR(g_EVP_DigestInit_ex2, "EVP_DigestInit_ex2");

    g_EVP_DigestUpdate = (T_EVP_DigestUpdate)dlsym(g_pOpenSSLHandle, "EVP_DigestUpdate");
    CHECK_FN_PTR(g_EVP_DigestUpdate, "EVP_DigestUpdate");

    g_EVP_DigestFinal_ex = (T_EVP_DigestFinal_ex)dlsym(g_pOpenSSLHandle, "EVP_DigestFinal_ex");
    CHECK_FN_PTR(g_EVP_DigestFinal_ex, "EVP_DigestFg_OpenSSLLoaderinal_ex");

    g_EVP_aes_256_cbc = (T_EVP_aes_256_cbc)dlsym(g_pOpenSSLHandle, "EVP_aes_256_cbc");
    CHECK_FN_PTR(g_EVP_aes_256_cbc, "EVP_aes_256_cbc");

    g_EVP_CIPHER_CTX_free = (T_EVP_CIPHER_CTX_free)dlsym(g_pOpenSSLHandle, "EVP_CIPHER_CTX_free");
    CHECK_FN_PTR(g_EVP_CIPHER_CTX_free, "EVP_CIPHER_CTX_free");

    g_EVP_CIPHER_CTX_new = (T_EVP_CIPHER_CTX_new)dlsym(g_pOpenSSLHandle, "EVP_CIPHER_CTX_new");
    CHECK_FN_PTR(g_EVP_CIPHER_CTX_new, "EVP_CIPHER_CTX_new");

    g_EVP_EncryptInit_ex = (T_EVP_EncryptInit_ex)dlsym(g_pOpenSSLHandle, "EVP_EncryptInit_ex");
    CHECK_FN_PTR(g_EVP_EncryptInit_ex, "EVP_EncryptInit_ex");

    g_EVP_EncryptUpdate = (T_EVP_EncryptUpdate)dlsym(g_pOpenSSLHandle, "EVP_EncryptUpdate");
    CHECK_FN_PTR(g_EVP_EncryptUpdate, "EVP_EncryptUpdate");

    g_EVP_EncryptFinal_ex = (T_EVP_EncryptFinal_ex)dlsym(g_pOpenSSLHandle, "EVP_EncryptFinal_ex");
    CHECK_FN_PTR(g_EVP_EncryptFinal_ex, "EVP_EncryptFinal_ex");

    g_EVP_DecryptInit_ex = (T_EVP_DecryptInit_ex)dlsym(g_pOpenSSLHandle, "EVP_DecryptInit_ex");
    CHECK_FN_PTR(g_EVP_DecryptInit_ex, "EVP_DecryptInit_ex");

    g_EVP_DecryptUpdate = (T_EVP_DecryptUpdate)dlsym(g_pOpenSSLHandle, "EVP_DecryptUpdate");
    CHECK_FN_PTR(g_EVP_DecryptUpdate, "EVP_DecryptUpdate");

    g_EVP_DecryptFinal_ex = (T_EVP_DecryptFinal_ex)dlsym(g_pOpenSSLHandle, "EVP_DecryptFinal_ex");
    CHECK_FN_PTR(g_EVP_DecryptFinal_ex, "EVP_DecryptFinal_ex");
}

struct COpenSSLLoaderAES
{
    COpenSSLLoaderAES()
    {
        try
        {
            LoadOpenSSL_AES();
        }
        catch(CZorgeError& e)
        {
            printf("%s\nExiting.\n\n", (const char*)e.GetMsg());
            exit(2024);
        }
    }
} g_OpenSSLLoaderAES;

std::atomic<unsigned int> g_CCoderAES = 0;
std::atomic<unsigned int> g_CCoderAES_max = 0;

void DiagCCoderAES(CZorgeString& strOut)
{
	CZorgeString str;
	str.Format("CCoderAES.\tCur=%d, Max=%d\n", (unsigned int)g_CCoderAES, (unsigned int)g_CCoderAES_max);
	strOut += str;
}

struct CCoderAESCounter
{
	~CCoderAESCounter()
	{
		printf("g_CCoderAES = %d (%d)\n", (unsigned int)g_CCoderAES, (unsigned int)g_CCoderAES_max);
	}
	static void Add()
	{
		g_CCoderAES++;
		g_CCoderAES_max++;
	}
} g_CCoderAESCounter;

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
CCoderAES::CCoderAES(bool bCom)
{
	CCoderAESCounter::Add();
	m_bCom = bCom;
	m_pCtxEncr = 0;
	m_pCtxDecr = 0;
	m_pCtxMd5 = g_EVP_MD_CTX_new();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
CCoderAES::~CCoderAES()
{
	g_CCoderAES--;
	if(m_pCtxEncr)
		g_EVP_CIPHER_CTX_free(m_pCtxEncr);

	if(m_pCtxDecr)
		g_EVP_CIPHER_CTX_free(m_pCtxDecr);

	if(m_pCtxMd5)
		g_EVP_MD_CTX_free(m_pCtxMd5);
}

// private
int CCoderAES::Init(unsigned char* szKey, size_t nKeySize, int nOper)
{
	if(nKeySize == 0)
	{
		printf("CCoderAES::Init, key size = 0, %d\n", __LINE__);
		return CODER_ERROR;
	}
	if(szKey == 0)
	{
		printf("CCoderAES::Init, szKey == 0, %d\n", __LINE__);
		return CODER_ERROR;
	}

	if(m_strKeyIV != (const char*)szKey)
	{
		m_strKeyIV = (const char*)szKey;
		//-----------------------------------------------------------------------
		// Calculate IV from the key
		//-----------------------------------------------------------------------
		if(!CalcMD5(szKey, nKeySize, m_IV))
		{
			printf("CCoderAES::Calc IV failed. %d", __LINE__);
			return CODER_ERROR;
		}

		// swap index 0 and 1
		unsigned char* pDataIV = m_IV.Get();
		if(!m_bCom)
		{
			unsigned char ch = pDataIV[1];
			pDataIV[1] = pDataIV[0];
			pDataIV[0] = ch;
		}

		//-----------------------------------------------------------------------
		// Calculate key based on IV
		//-----------------------------------------------------------------------
		unsigned char* pDataKey = m_Key.Allocate(KEY_SIZE);
		memcpy(pDataKey, pDataIV, IV_SIZE);
		memcpy(pDataKey + IV_SIZE, pDataIV, IV_SIZE);
		m_Key.SetSizeData(KEY_SIZE);
	}

	int nRc = 0;
	//-----------------------------------------------------------------------
	// Context for encrypt
	//-----------------------------------------------------------------------
	if(nOper == OPER_ENCR)
	{
		if(m_pCtxEncr)
		{
			g_EVP_CIPHER_CTX_free(m_pCtxEncr);
			m_pCtxEncr = 0;
		}

		if(!(m_pCtxEncr = g_EVP_CIPHER_CTX_new()))
		{
			printf("Assert m_pCtxEncr = EVP_CIPHER_CTX_new() in CCoderAES::Init %d\n", __LINE__);
			return CODER_ERROR;
		}

		nRc = g_EVP_EncryptInit_ex(m_pCtxEncr, g_EVP_aes_256_cbc(), NULL, m_Key.Get(), m_IV.Get());
		if(nRc != CODER_OK)
			return nRc;
	}
	else if(nOper == OPER_DECR)
	{
		//-----------------------------------------------------------------------
		// Context for decrypt
		//-----------------------------------------------------------------------
		if(m_pCtxDecr)
		{
			g_EVP_CIPHER_CTX_free(m_pCtxDecr);
			m_pCtxDecr = 0;
		}

		if(!(m_pCtxDecr = g_EVP_CIPHER_CTX_new()))
		{
			printf("Assert m_pCtxDecr = EVP_CIPHER_CTX_new() in CCoderAES::Init, %d\n", __LINE__);
			return CODER_ERROR;
		}

		nRc = g_EVP_DecryptInit_ex(m_pCtxDecr, g_EVP_aes_256_cbc(), NULL, m_Key.Get(), m_IV.Get());
		if(nRc != CODER_OK)
			return nRc;
	}

	return CODER_OK;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void CCoderAES::Init(const CZorgeString& strPsw)
{
	if(strPsw.IsEmpty())
		throw CZorgeAssert(" Assert(strPsw.IsEmpty())");
	m_strKey = strPsw;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int CCoderAES::EncryptToHexChars(const CZorgeString& strInput, CZorgeString& strOut)
{
	CMemBuffer Buf;
	unsigned char* szInput = (unsigned char*)(const char*)strInput;
	size_t nInput = strInput.GetLength();
	int	nRc = Encrypt(szInput, nInput, Buf);
	if(nRc != CODER_OK)
		return nRc;

	strOut.EmptyData();
	size_t nSize = Buf.GetSizeData();
	unsigned char* pData = Buf.Get();

	char* szHex = new char[nSize * 2 + 1];
	const char* szByte = 0;

	char* szHexPtr = szHex;
	for(size_t i = 0; i < nSize && pData; ++i, ++pData)
	{
		szByte = ByteToHexStr(*pData);
		*szHexPtr++ = szByte[0];
		*szHexPtr++ = szByte[1];
	}
	*szHexPtr = 0;

	strOut = szHex;
	delete [] szHex;
	return nRc;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int CCoderAES::DecryptFromHexChars(const CZorgeString& strInput, CZorgeString& strOut)
{
	unsigned int nInputLen = strInput.GetLength();
	if(nInputLen % 2 != 0)
	{
		printf("Invalid input len %d, %d\n", nInputLen, __LINE__);
		return CODER_ERROR;
	}
	int nPairs = nInputLen / 2;

	CMemBuffer BufInput(nPairs + 1);

	const char* szInput = (const char*)strInput;
	unsigned char* szBuf = BufInput.Get();
	char chLeft(0), chRight(0);
	for(int i = 0; i < nPairs; ++i)
	{
		chLeft = *szInput++;
		chRight = *szInput++;
		unsigned char ch = HexCharsToByte(chLeft, chRight);
		*szBuf++ = ch;
	}
	*szBuf = 0;
	BufInput.SetSizeData(nPairs);

	CMemBuffer BufOut;
	int	nRc = Decrypt(BufInput.Get(), nPairs, BufOut);
	if(nRc == CODER_OK)
	{
		BufOut.Get()[BufOut.GetSizeData()] = 0;
		strOut = (const char*)BufOut.Get();
	}
	return nRc;
}

// private
int	CCoderAES::Encrypt(unsigned char* szPlain, size_t nPlainSize, CMemBuffer& Out)
{
	Init((unsigned char*)(const char*)m_strKey, m_strKey.GetLength(), OPER_ENCR);

	if(m_pCtxEncr == 0)
		return CODER_ERROR;

	size_t nNewLen = nPlainSize + AES_BLOCK_SIZE;
	unsigned char* pCipherText = Out.Allocate(nNewLen);
	int nLen = 0;
    int nRc = g_EVP_EncryptUpdate(m_pCtxEncr, pCipherText, &nLen, szPlain, nPlainSize);
    if(nRc != CODER_OK)
    	return nRc;

    int nCipherTextLen = nLen;
    nRc = g_EVP_EncryptFinal_ex(m_pCtxEncr, pCipherText + nLen, &nLen);
    if(nRc != CODER_OK)
    	return nRc;

    nCipherTextLen += nLen;
    if(!Out.SetSizeData(nCipherTextLen))
    {
    	printf("Assert nCipherTextLen = %d, %d\n", nCipherTextLen, __LINE__);
    	return CODER_ERROR;
    }

	return CODER_OK;
}

// private
int	CCoderAES::Encrypt(const CMemBuffer& In, CMemBuffer& Out)
{
	return Encrypt(In.Get(), In.GetSizeData(), Out);
}

// private
int	CCoderAES::Decrypt(unsigned char* szEncr, size_t nEncrSize, CMemBuffer& Out)
{
	Init((unsigned char*)(const char*)m_strKey, m_strKey.GetLength(), OPER_DECR);

	if(m_pCtxDecr == 0)
		return -1;

	unsigned char* pPlainText = Out.Allocate(nEncrSize);
	int nLen = 0;
	int nRc = g_EVP_DecryptUpdate(m_pCtxDecr, pPlainText, &nLen, szEncr, nEncrSize);
	if(nRc != CODER_OK)
        return nRc;

	int nPlainTextLen = nLen;
    nRc = g_EVP_DecryptFinal_ex(m_pCtxDecr, pPlainText + nLen, &nLen);
    if(nRc != CODER_OK)
        return nRc;

    nPlainTextLen += nLen;
    if(!Out.SetSizeData(nPlainTextLen))
    {
    	printf("Assert nPlainTextLen=%d, %d\n", nPlainTextLen, __LINE__);
    	return -2;
    }

	return CODER_OK;
}

// private
int	CCoderAES::Decrypt(const CMemBuffer& In, CMemBuffer& Out)
{
	return Decrypt(In.Get(), In.GetSizeData(), Out);
}

// https://stackoverflow.com/questions/7860362/how-can-i-use-openssl-md5-in-c-to-hash-a-string
// private
bool CCoderAES::CalcMD5(unsigned char* pData, size_t nDataSize, CMemBuffer& Out)
{
	if(nDataSize == 0)
		return false;

	Out.SetSizeData(0);
	g_EVP_DigestInit_ex2(m_pCtxMd5, g_EVP_md5(), NULL);
	g_EVP_DigestUpdate(m_pCtxMd5, pData, nDataSize);

	unsigned char* pOut = Out.Allocate(IV_SIZE);
	unsigned int nLen = 0;
	g_EVP_DigestFinal_ex(m_pCtxMd5, pOut, &nLen);
	if(nLen != IV_SIZE)
		printf("CCoderAES::CalcMD5 %d != %d, %d\n", nLen, IV_SIZE, __LINE__);
	Out.SetSizeData(nLen);

	return true;
}

/*
struct CT
{
	CT()
	{
		CCoderAES c(true);
		CZorgeString strKey("key1"), strData("Hello world!");

		c.Init(strKey);

		CMemBuffer b1, b2;
		unsigned char* sz = (unsigned char*)(const char*)strData;
		b1.Init(sz, (size_t)strData.GetLength());

		c.Encrypt(b1, b2);

		puts("---> b2");
		b2.Print();
		puts("<--- b2");

		CMemBuffer d1;
		c.Decrypt(b2, d1);
		puts("---> d1");
		d1.Print();
		puts("<--- d1");
	}
} gt1;
*/
