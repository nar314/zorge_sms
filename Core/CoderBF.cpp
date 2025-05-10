#include "CoderBF.h"

#include <dlfcn.h>
// sudo apt-get install libssl-dev

#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

#include <atomic>

#include "ZorgeError.h"
#include "StrUtils.h"

#define MD5_SIZE 	16

typedef EVP_MD_CTX* (*T_EVP_MD_CTX_new)(void);
typedef const EVP_MD* (*T_EVP_md5)(void);
typedef void (*T_EVP_MD_CTX_free)(EVP_MD_CTX*);
typedef void (*T_BF_set_key)(BF_KEY*, int, const unsigned char*);
typedef int (*T_EVP_DigestInit_ex2)(EVP_MD_CTX*, const EVP_MD*, const OSSL_PARAM []);
typedef int (*T_EVP_DigestUpdate)(EVP_MD_CTX*, const void*, size_t);
typedef int (*T_EVP_DigestFinal_ex)(EVP_MD_CTX*, unsigned char*, unsigned int*);

typedef void (*T_BF_cfb64_encrypt)(const unsigned char*, unsigned char*, long, const BF_KEY*, unsigned char*, int*, int);

// MD5
static T_EVP_md5 g_EVP_md5 = 0;
static T_EVP_MD_CTX_free g_EVP_MD_CTX_free = 0;
static T_EVP_MD_CTX_new g_EVP_MD_CTX_new = 0;
static T_BF_set_key g_BF_set_key = 0;
static T_EVP_DigestInit_ex2 g_EVP_DigestInit_ex2 = 0;
static T_EVP_DigestUpdate g_EVP_DigestUpdate = 0;
static T_EVP_DigestFinal_ex g_EVP_DigestFinal_ex = 0;

static T_BF_cfb64_encrypt g_BF_cfb64_encrypt = 0;

void* g_pOpenSSLHandle_BF = 0;

#define LIB_CRYPTO_NAME "libcrypto.so"

#define CHECK_FN_PTR(p, n) if(!p) { CZorgeString s; s.Format("Failed to load openSSL function '%s'", n); throw CZorgeError(s);}
static void LoadOpenSSL_BF()
{
    if(g_pOpenSSLHandle_BF)
        return;

    g_pOpenSSLHandle_BF = dlopen(LIB_CRYPTO_NAME, RTLD_NOW); // I never close this handle.
    if(g_pOpenSSLHandle_BF == 0)
    {
		CZorgeString strMsg;
		strMsg.Format("BF. Failed to load openSSL library '%s'", LIB_CRYPTO_NAME);
		throw CZorgeError(strMsg);
    }

    g_EVP_MD_CTX_free = (T_EVP_MD_CTX_free)dlsym(g_pOpenSSLHandle_BF, "EVP_MD_CTX_free");
    CHECK_FN_PTR(g_EVP_MD_CTX_free, "EVP_MD_CTX_free");

    g_EVP_md5 = (T_EVP_md5)dlsym(g_pOpenSSLHandle_BF, "EVP_md5");
    CHECK_FN_PTR(g_EVP_md5, "EVP_md5");

    g_EVP_MD_CTX_new = (T_EVP_MD_CTX_new)dlsym(g_pOpenSSLHandle_BF, "EVP_MD_CTX_new");
    CHECK_FN_PTR(g_EVP_MD_CTX_new, "EVP_MD_CTX_new");

    g_BF_set_key = (T_BF_set_key)dlsym(g_pOpenSSLHandle_BF, "BF_set_key");
    CHECK_FN_PTR(g_BF_set_key, "BF_set_key");

    g_EVP_DigestInit_ex2 = (T_EVP_DigestInit_ex2)dlsym(g_pOpenSSLHandle_BF, "EVP_DigestInit_ex2");
    CHECK_FN_PTR(g_EVP_DigestInit_ex2, "EVP_DigestInit_ex2");

    g_EVP_DigestUpdate = (T_EVP_DigestUpdate)dlsym(g_pOpenSSLHandle_BF, "EVP_DigestUpdate");
    CHECK_FN_PTR(g_EVP_DigestUpdate, "EVP_DigestUpdate");

    g_EVP_DigestFinal_ex = (T_EVP_DigestFinal_ex)dlsym(g_pOpenSSLHandle_BF, "EVP_DigestFinal_ex");
    CHECK_FN_PTR(g_EVP_DigestFinal_ex, "EVP_DigestFinal_ex");

    g_BF_cfb64_encrypt = (T_BF_cfb64_encrypt)dlsym(g_pOpenSSLHandle_BF, "BF_cfb64_encrypt");
    CHECK_FN_PTR(g_BF_cfb64_encrypt, "BF_cfb64_encrypt");
}

struct COpenSSLLoader_BF
{
    COpenSSLLoader_BF()
    {
        try
        {
            LoadOpenSSL_BF();
        }
        catch(CZorgeError& e)
        {
            printf("%s\nExiting.\n\n", (const char*)e.GetMsg());
            exit(2023);
        }
    }
} g_OpenSSLLoaderBF;

std::atomic<unsigned int> g_CCoderBF = 0;
std::atomic<unsigned int> g_CCoderBF_max = 0;

void DiagCCoderBF(CZorgeString& strOut)
{
	CZorgeString str;
	str.Format("CCoderBF.\tCur=%d, Max=%d\n", (unsigned int)g_CCoderBF, (unsigned int)g_CCoderBF_max);
	strOut += str;
}

struct CCoderBFCounter
{
	~CCoderBFCounter()
	{
		printf("CCoderBF = %d (%d)\n", (unsigned int)g_CCoderBF, (unsigned int)g_CCoderBF_max);
	}
} g_CCoderBFCounter;

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
CCoderBF::CCoderBF()
{
	g_CCoderBF++;
	g_CCoderBF_max++;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
CCoderBF::~CCoderBF()
{
	g_CCoderBF--;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void CCoderBF::Init(const CZorgeString& strPsw)
{
	if(strPsw.IsEmpty())
		throw CZorgeError("Password can not be empty.");

	CMemBuffer BufMD5;
	unsigned char* szPsw = (unsigned char*)(const char*)strPsw;
	int nPswSize = strPsw.GetLength();
	if(!CalcMD5(szPsw, (size_t)nPswSize, BufMD5))
		throw CZorgeAssert("Assert encrypt");

	if(BufMD5.GetSizeData() < BF_IV_SIZE)
		throw CZorgeAssert("Assert size");

	// Using 8 out of 16 bytes of MD5
	memcpy(m_IV, BufMD5.Get(), BF_IV_SIZE);
	g_BF_set_key(&m_Key, nPswSize, szPsw);
}

// private
bool CCoderBF::CalcMD5(unsigned char* pData, size_t nDataSize, CMemBuffer& Out)
{
	if(nDataSize == 0)
		return false;

	Out.SetSizeData(0);
	EVP_MD_CTX* pCtxMd5 = g_EVP_MD_CTX_new();
	g_EVP_DigestInit_ex2(pCtxMd5, g_EVP_md5(), NULL);
	g_EVP_DigestUpdate(pCtxMd5, pData, nDataSize);

	unsigned char* pOut = Out.Allocate(MD5_SIZE);
	unsigned int nLen = 0;

	g_EVP_DigestFinal_ex(pCtxMd5, pOut, &nLen);
	if(nLen != MD5_SIZE)
		printf("CCoderBF::CalcMD5 %d != %d, %d\n", nLen, MD5_SIZE, __LINE__);
	Out.SetSizeData(nLen);

	g_EVP_MD_CTX_free(pCtxMd5);
	return true;
}

// private
void CCoderBF::Encrypt(unsigned char* pData, size_t nDataLen, CMemBuffer& Out)
{
	if(nDataLen == 0)
		throw CZorgeError("Encrypt. Data is empty.");

	unsigned char IV[BF_IV_SIZE];
	memcpy(IV, m_IV, BF_IV_SIZE);

	int nCount(0);
	unsigned char* pEncr = Out.Allocate(nDataLen);
	g_BF_cfb64_encrypt(pData, pEncr, nDataLen, &m_Key, IV, &nCount, BF_ENCRYPT);
	Out.SetSizeData(nDataLen);
}

// private
void CCoderBF::Decrypt(unsigned char* pData, size_t nDataLen, CMemBuffer& Out)
{
	if(nDataLen == 0)
		throw CZorgeError("Decrypt. Data is empty.");

	unsigned char IV[BF_IV_SIZE];
	memcpy(IV, m_IV, BF_IV_SIZE);

	int nCount(0);
	unsigned char* pDecr = Out.Allocate(nDataLen);
	g_BF_cfb64_encrypt(pData, pDecr, nDataLen, &m_Key, IV, &nCount, BF_DECRYPT);
	Out.SetSizeData(nDataLen);
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void CCoderBF::EncryptToHexChars(const CZorgeString& strInput, CZorgeString& strOut)
{
	CMemBuffer Buf;
	unsigned char* szInput = (unsigned char*)(const char*)strInput;
	size_t nInput = strInput.GetLength();
	Encrypt(szInput, nInput, Buf);

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
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void CCoderBF::DecryptFromHexChars(const CZorgeString& strInput, CZorgeString& strOut)
{
	unsigned int nInputLen = strInput.GetLength();
	if(nInputLen % 2 != 0)
	{
		printf("Invalid input len %d, %d\n", nInputLen, __LINE__);
		throw CZorgeError("Invalid input.");
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
	Decrypt(BufInput.Get(), nPairs, BufOut);
	BufOut.Get()[BufOut.GetSizeData()] = 0;
	strOut = (const char*)BufOut.Get();
}
/*
void F1()
{
	CCoderBF CoderBF;
	CoderBF.Init("1234567-12345678-12345678");

	CZorgeString strOne, strEncr;
	for(int n = 0; n <= 255; ++n)
	{
		strOne.Format("%02X", n);
		CoderBF.EncryptToHexChars(strOne, strEncr);
		printf("%s %s\n", (const char*)strOne, (const char*)strEncr);
	}
}

struct C1
{
	C1()
	{
		F1();
	}
} fx;
*/
