
#include "CoderRSA.h"
#include "ZorgeError.h"

#include <dlfcn.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>

//#include <string.h>
#include <atomic>

// https://h41379.www4.hpe.com/doc/83final/ba554_90007/index.html
// https://docs.openssl.org/3.1/man3/X509_sign/#description

typedef void (*T_BN_clear)(BIGNUM*);
typedef void (*T_BN_free)(BIGNUM*);
typedef void (*T_RSA_free)(RSA*);
typedef unsigned long (*T_ERR_get_error)(void);
typedef char* (*T_ERR_error_string)(unsigned long e, char *buf);
typedef const BIO_METHOD* (*T_BIO_s_mem)(void);
typedef BIO* (*T_BIO_new)(const BIO_METHOD *type);
typedef EVP_PKEY* (*T_EVP_PKEY_new)(void);
typedef int (*T_EVP_PKEY_assign)(EVP_PKEY *pkey, int type, void *key);
typedef X509* (*T_X509_new)();
typedef ASN1_INTEGER* (*T_X509_get_serialNumber)(X509 *x);
typedef int (*T_ASN1_INTEGER_set)(ASN1_INTEGER *a, long v);
typedef ASN1_TIME* (*T_X509_gmtime_adj)(ASN1_TIME *s, long adj);
typedef ASN1_TIME* (*T_X509_getm_notBefore)(const X509 *x);
typedef ASN1_TIME* (*T_X509_getm_notAfter)(const X509 *x);
typedef const EVP_MD* (*T_EVP_sha1)(void);
typedef int (*T_X509_sign)(X509 *x, EVP_PKEY *pkey, const EVP_MD *md);
typedef int (*T_X509_set_pubkey)(X509 *x, EVP_PKEY *pkey);
typedef EVP_PKEY* (*T_X509_get_pubkey)(X509 *x);
typedef int (*T_PEM_write_bio_PUBKEY)(BIO*, const EVP_PKEY*);
typedef int (*T_BIO_read)(BIO *b, void *data, int dlen);
typedef int (*T_BIO_free)(BIO *a);
typedef void (*T_EVP_PKEY_free)(EVP_PKEY *pkey);
typedef void (*T_X509_free)(X509*);
typedef BIGNUM* (*T_BN_new)(void);
typedef int (*T_BN_set_word)(BIGNUM *a, BN_ULONG w);
typedef RSA* (*T_RSA_new)(void);
typedef int (*T_RSA_generate_key_ex)(RSA *rsa, int bits, BIGNUM *e, BN_GENCB *cb);
typedef int (*T_RSA_size)(const RSA *rsa);
typedef int (*T_RSA_public_encrypt)(int flen, const unsigned char *from, unsigned char *to, RSA *rsa, int padding);
typedef int (*T_RSA_private_decrypt)(int flen, const unsigned char *from, unsigned char *to, RSA *rsa, int padding);
typedef int (*T_OPENSSL_init_crypto)(uint64_t opts, const OPENSSL_INIT_SETTINGS *settings);

static void* g_pOpenSSLHandle = 0;

static T_BN_clear g_BN_clear = 0;
static T_BN_free g_BN_free = 0;
static T_RSA_free g_RSA_free = 0;
static T_ERR_get_error g_ERR_get_error = 0;
static T_ERR_error_string g_ERR_error_string = 0;
static T_BIO_s_mem g_BIO_s_mem = 0;
static T_BIO_new g_BIO_new = 0;
static T_EVP_PKEY_new g_EVP_PKEY_new = 0;
static T_EVP_PKEY_assign g_EVP_PKEY_assign = 0;
static T_X509_new g_X509_new = 0;
static T_X509_get_serialNumber g_X509_get_serialNumber = 0;
static T_ASN1_INTEGER_set g_ASN1_INTEGER_set = 0;
static T_X509_gmtime_adj g_X509_gmtime_adj = 0;
static T_X509_getm_notBefore g_X509_getm_notBefore = 0;
static T_X509_getm_notAfter g_X509_getm_notAfter = 0;
static T_EVP_sha1 g_EVP_sha1 = 0;
static T_X509_sign g_X509_sign = 0;
static T_X509_set_pubkey g_X509_set_pubkey = 0;
static T_X509_get_pubkey g_X509_get_pubkey = 0;
static T_PEM_write_bio_PUBKEY g_PEM_write_bio_PUBKEY = 0;
static T_BIO_read g_BIO_read = 0;
static T_BIO_free g_BIO_free = 0;
static T_EVP_PKEY_free g_EVP_PKEY_free = 0;
static T_X509_free g_X509_free = 0;
static T_BN_new g_BN_new = 0;
static T_BN_set_word g_BN_set_word = 0;
static T_RSA_new g_RSA_new = 0;
static T_RSA_generate_key_ex g_RSA_generate_key_ex = 0;
static T_RSA_size g_RSA_size = 0;
static T_RSA_public_encrypt g_RSA_public_encrypt = 0;
static T_RSA_private_decrypt g_RSA_private_decrypt = 0;
static T_OPENSSL_init_crypto g_OPENSSL_init_crypto = 0;

#define LIB_CRYPTO_NAME "libcrypto.so"
#define CHECK_FN_PTR(p, n) if(!p) { CZorgeString s; s.Format("Failed to load openSSL function '%s'", n); throw CZorgeError(s);}
static void LoadOpenSSL_RSA()
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

    g_BN_clear = (T_BN_clear)dlsym(g_pOpenSSLHandle, "BN_clear");
    CHECK_FN_PTR(g_BN_clear, "BN_clear");

    g_BN_free = (T_BN_free)dlsym(g_pOpenSSLHandle, "BN_free");
    CHECK_FN_PTR(g_BN_free, "BN_free");

    g_RSA_free = (T_RSA_free)dlsym(g_pOpenSSLHandle, "RSA_free");
    CHECK_FN_PTR(g_RSA_free, "RSA_free");

    g_ERR_get_error = (T_ERR_get_error)dlsym(g_pOpenSSLHandle, "ERR_get_error");
    CHECK_FN_PTR(g_ERR_get_error, "ERR_get_error");

    g_ERR_error_string = (T_ERR_error_string)dlsym(g_pOpenSSLHandle, "ERR_error_string");
    CHECK_FN_PTR(g_ERR_error_string, "ERR_error_string");

    g_BIO_s_mem = (T_BIO_s_mem)dlsym(g_pOpenSSLHandle, "BIO_s_mem");
    CHECK_FN_PTR(g_BIO_s_mem, "BIO_s_mem");

    g_BIO_new = (T_BIO_new)dlsym(g_pOpenSSLHandle, "BIO_new");
    CHECK_FN_PTR(g_BIO_new, "BIO_new");

    g_EVP_PKEY_new = (T_EVP_PKEY_new)dlsym(g_pOpenSSLHandle, "EVP_PKEY_new");
    CHECK_FN_PTR(g_EVP_PKEY_new, "EVP_PKEY_new");

    g_EVP_PKEY_assign = (T_EVP_PKEY_assign)dlsym(g_pOpenSSLHandle, "EVP_PKEY_assign");
    CHECK_FN_PTR(g_EVP_PKEY_assign, "EVP_PKEY_assign");

    g_X509_new = (T_X509_new)dlsym(g_pOpenSSLHandle, "X509_new");
    CHECK_FN_PTR(g_X509_new, "X509_new");

    g_X509_get_serialNumber = (T_X509_get_serialNumber)dlsym(g_pOpenSSLHandle, "X509_get_serialNumber");
    CHECK_FN_PTR(g_X509_get_serialNumber, "X509_get_serialNumber");

    g_ASN1_INTEGER_set = (T_ASN1_INTEGER_set)dlsym(g_pOpenSSLHandle, "ASN1_INTEGER_set");
    CHECK_FN_PTR(g_ASN1_INTEGER_set, "ASN1_INTEGER_set");

    g_X509_gmtime_adj = (T_X509_gmtime_adj)dlsym(g_pOpenSSLHandle, "X509_gmtime_adj");
    CHECK_FN_PTR(g_X509_gmtime_adj, "X509_gmtime_adj");

    g_X509_getm_notBefore = (T_X509_getm_notBefore)dlsym(g_pOpenSSLHandle, "X509_getm_notBefore");
    CHECK_FN_PTR(X509_getm_notBefore, "X509_getm_notBefore");

    g_X509_getm_notAfter = (T_X509_getm_notAfter)dlsym(g_pOpenSSLHandle, "X509_getm_notAfter");
    CHECK_FN_PTR(g_X509_getm_notAfter, "X509_getm_notAfter");

    g_EVP_sha1 = (T_EVP_sha1)dlsym(g_pOpenSSLHandle, "EVP_sha1");
    CHECK_FN_PTR(g_EVP_sha1, "EVP_sha1");

    g_X509_sign = (T_X509_sign)dlsym(g_pOpenSSLHandle, "X509_sign");
    CHECK_FN_PTR(g_X509_sign, "X509_sign");

    g_X509_set_pubkey = (T_X509_set_pubkey)dlsym(g_pOpenSSLHandle, "X509_set_pubkey");
    CHECK_FN_PTR(g_X509_set_pubkey, "X509_set_pubkey");

    g_X509_get_pubkey = (T_X509_get_pubkey)dlsym(g_pOpenSSLHandle, "X509_get_pubkey");
    CHECK_FN_PTR(g_X509_set_pubkey, "X509_get_pubkey");

    g_PEM_write_bio_PUBKEY = (T_PEM_write_bio_PUBKEY)dlsym(g_pOpenSSLHandle, "PEM_write_bio_PUBKEY");
    CHECK_FN_PTR(g_PEM_write_bio_PUBKEY, "PEM_write_bio_PUBKEY");

    g_BIO_read = (T_BIO_read)dlsym(g_pOpenSSLHandle, "BIO_read");
    CHECK_FN_PTR(g_BIO_read, "BIO_read");

    g_BIO_free = (T_BIO_free)dlsym(g_pOpenSSLHandle, "BIO_free");
    CHECK_FN_PTR(g_BIO_free, "BIO_free");

    g_EVP_PKEY_free = (T_EVP_PKEY_free)dlsym(g_pOpenSSLHandle, "EVP_PKEY_free");
    CHECK_FN_PTR(g_EVP_PKEY_free, "EVP_PKEY_free");

    g_X509_free = (T_X509_free)dlsym(g_pOpenSSLHandle, "X509_free");
    CHECK_FN_PTR(g_X509_free, "X509_free");

    g_BN_new = (T_BN_new)dlsym(g_pOpenSSLHandle, "BN_new");
    CHECK_FN_PTR(g_BN_new, "BN_new");

    g_BN_set_word = (T_BN_set_word)dlsym(g_pOpenSSLHandle, "BN_set_word");
    CHECK_FN_PTR(g_BN_set_word, "BN_set_word");

    g_RSA_new = (T_RSA_new)dlsym(g_pOpenSSLHandle, "RSA_new");
    CHECK_FN_PTR(g_RSA_new, "RSA_new");

    g_RSA_generate_key_ex = (T_RSA_generate_key_ex)dlsym(g_pOpenSSLHandle, "RSA_generate_key_ex");
    CHECK_FN_PTR(g_RSA_generate_key_ex, "RSA_generate_key_ex");

    g_RSA_size = (T_RSA_size)dlsym(g_pOpenSSLHandle, "RSA_size");
    CHECK_FN_PTR(g_RSA_size, "RSA_size");

    g_RSA_public_encrypt = (T_RSA_public_encrypt)dlsym(g_pOpenSSLHandle, "RSA_public_encrypt");
    CHECK_FN_PTR(g_RSA_public_encrypt, "RSA_public_encrypt");

    g_RSA_private_decrypt = (T_RSA_private_decrypt)dlsym(g_pOpenSSLHandle, "RSA_private_decrypt");
    CHECK_FN_PTR(g_RSA_private_decrypt, "RSA_private_decrypt");

    g_OPENSSL_init_crypto = (T_OPENSSL_init_crypto)dlsym(g_pOpenSSLHandle, "OPENSSL_init_crypto");
    CHECK_FN_PTR(g_OPENSSL_init_crypto, "OPENSSL_init_crypto");
}

struct COpenSSLLoaderRSA
{
    COpenSSLLoaderRSA()
    {
        try
        {
            LoadOpenSSL_RSA();
        }
        catch(CZorgeError& e)
        {
            printf("%s\nExiting.\n\n", (const char*)e.GetMsg());
            exit(2025);
        }
    }
} g_OpenSSLLoaderRSA;

std::atomic<unsigned int> g_CCoderRSA = 0;
std::atomic<unsigned int> g_CCoderRSA_max = 0;

void DiagCCoderRSA(CZorgeString& strOut)
{
	CZorgeString str;
	str.Format("CCoderRSA.\tCur=%d, Max=%d\n", (unsigned int)g_CCoderRSA, (unsigned int)g_CCoderRSA_max);
	strOut += str;
}

struct CCoderRSACounter
{
	~CCoderRSACounter()
	{
		printf("CCoderRSA = %d (%d)\n", (unsigned int)g_CCoderRSA, (unsigned int)g_CCoderRSA_max);
	}
} g_CCoderRSACounter;

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
CCoderRSA::CCoderRSA()
{
	m_pBigNum = 0;
	m_pRSA = 0;
	m_szError = 0;
}

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
CCoderRSA::~CCoderRSA()
{
	Clear();
}

// private
void CCoderRSA::Clear()
{
	if(m_pBigNum)
	{
		g_BN_clear(m_pBigNum);
		g_BN_free(m_pBigNum);
		m_pBigNum = 0;
	}
	if(m_pRSA)
	{
		g_RSA_free(m_pRSA);
		m_pRSA = 0;
	}
	if(m_szError)
	{
		delete [] m_szError;
		m_szError = 0;
	}
	m_strPublicKey.Empty();
}

// private
char* CCoderRSA::GetError()
{
	if(!m_szError)
	{
		m_szError = new char[128];
		m_szError[0] = 0;
		//ERR_load_crypto_strings();
		g_OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
	}
	g_ERR_error_string(g_ERR_get_error(), m_szError);
	return m_szError;
}

// private
void CCoderRSA::GeneratePublicKey_X509()
{
	m_strPublicKey.EmptyData();

	BIO* pBio = 0;
	EVP_PKEY* pEvpKey = 0;
	X509* pX509 = 0;
	try
	{
		// Create abstract IO for use in memory.
		pBio = g_BIO_new(g_BIO_s_mem());
		if(!pBio)
			throw CZorgeAssert(GetError());

		// Create EVP from RSA
		pEvpKey = g_EVP_PKEY_new();
		int nRc = g_EVP_PKEY_assign(pEvpKey, EVP_PKEY_RSA, m_pRSA);
		if(nRc != 1)
			throw CZorgeAssert(GetError());

		// Create X509 from EVP
		pX509 = g_X509_new();
		g_ASN1_INTEGER_set(g_X509_get_serialNumber(pX509), 1);
		g_X509_gmtime_adj(g_X509_getm_notBefore(pX509), 0);
		g_X509_gmtime_adj(g_X509_getm_notAfter(pX509), 31536000L);
		g_X509_sign(pX509, pEvpKey, g_EVP_sha1());

		nRc = g_X509_set_pubkey(pX509, pEvpKey);
		if(nRc == 0)
			throw CZorgeAssert(GetError());

		// Get X509 public key
		EVP_PKEY* pPubKey = g_X509_get_pubkey(pX509);
		if(!pPubKey)
			throw CZorgeAssert(GetError());

		nRc = g_PEM_write_bio_PUBKEY(pBio, pPubKey);
		if(nRc == 0)
			throw CZorgeAssert(GetError());

		// Read X509 public key into memory buffer
		int nBufSize = 8192;
		char Buf[nBufSize];
		memset(Buf, 0, nBufSize);

		nRc = g_BIO_read(pBio, Buf, nBufSize);
		if(nRc < 1)
			throw CZorgeAssert(GetError());
	/*
		-----BEGIN PUBLIC KEY-----
		MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArqL2mcSIG1aTZOdSoHF4
		18+ZlRbfOiJEW7QZsl+ekZ55kncA4aOo2miLfSbEW0yYuU0520PsqB+lVedWl2Az
		2G+ZeeG96jXXHZquwIQRWSREyIdWDBQy2g7NTmvq57PTx/5l18YoIETOdgJuHf33
		Fm+I37E0ClHR5K1a/IG9gjNGSVQUvXwpem1LPs4lHBGl/EhCrCGWQGjFjFtGmqs4
		a5TY8xnhan9Lbt7Lii7iD5vbdqC7X+E19WtRGGEwFoX+whKHXlIr8zA5QjJz00lg
		xFk8nBdo/Rh6QpaKqDMc3lhc92BrmhmPcNHslCLMiFqgBzmFaDuT1niENiL0K8+g
		IQIDAQAB
		-----END PUBLIC KEY-----
	*/
		// Remove prefix, suffix and \r\n
		#define PREFIX_LEN 26
		#define SUFFIX_LEN 24
		if(memcmp(Buf, "-----BEGIN PUBLIC KEY-----", PREFIX_LEN) != 0)
		{
			CZorgeString strErr;
			strErr = "Unexpected prefix in the public key:"; strErr += Buf;
			throw CZorgeError(strErr);
		}
		int nLen = strlen(Buf);
		char* pSuf = Buf + (nLen - SUFFIX_LEN - 1);
		if(memcmp(pSuf, "-----END PUBLIC KEY-----", SUFFIX_LEN) != 0)
		{
			CZorgeString strErr;
			strErr = "Unexpected suffix in the public key:"; strErr += Buf;
			throw CZorgeError(strErr);
		}

		m_strPublicKey.GetBuffer(nLen);
		char* pFrom = Buf + PREFIX_LEN;
		char* pTo = pSuf;
		char ch = 0;
		for(; pFrom != pTo; ++pFrom)
		{
			ch = *pFrom;
			if(ch == 10 || ch == 13)
				continue;
			m_strPublicKey += ch;
		}
		g_BIO_free(pBio);
		g_EVP_PKEY_free(pEvpKey);
		g_X509_free(pX509);
	}
	catch(ALL_ERRORS& e)
	{
		if(pBio)
			g_BIO_free(pBio);

		if(pEvpKey)
			g_EVP_PKEY_free(pEvpKey);

		if(pX509)
			g_X509_free(pX509);
	}
}

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void CCoderRSA::Init()
{
	CMutexGuard G(m_Lock);

	if(m_pBigNum || m_pRSA)
		Clear();

	m_pBigNum = g_BN_new();
	if(!m_pBigNum)
		throw CZorgeError(GetError());

	int nRc = g_BN_set_word(m_pBigNum, 65537);
	if(nRc != 1)
		throw CZorgeError(GetError());

	m_pRSA = g_RSA_new();
	if(!m_pRSA)
		throw CZorgeError(GetError());

	int nKeyLenBits = 2048; // 2048 will use 256 bytes long key.
	nRc = g_RSA_generate_key_ex(m_pRSA, nKeyLenBits, m_pBigNum, 0);
	if(nRc != 1)
		throw CZorgeError(GetError());

	// Get public key for client. It has to be in form of X509 certificate.
	GeneratePublicKey_X509();
}

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
const CZorgeString&	CCoderRSA::GetPublicKey() const
{
	return m_strPublicKey;
}

// key 1024, max input 117, output = 128
// key 2048. max input 245, putput = 256
//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void CCoderRSA::Encrypt(const unsigned char* szInput, int nInputLen, CMemBuffer& Out)
{
	// Empty string can be encrypted. Do not check szInput == 0
	if(nInputLen < 0)
		throw CZorgeError("Invalid input length.");
	if(!m_pRSA)
		throw CZorgeError("RSA not initialized.");

	CMutexGuard G(m_Lock);
	int nBlockSize = g_RSA_size(m_pRSA);
	if(nBlockSize < 1)
		throw CZorgeAssert(GetError());

	if(nInputLen > nBlockSize - 12) // 11 is an actual value.
		throw CZorgeAssert("Input is too long.");

	unsigned char* pEncrypted = Out.Allocate(nBlockSize);

	int nRc = g_RSA_public_encrypt(nInputLen, szInput, pEncrypted, m_pRSA, RSA_PKCS1_PADDING);
	if(nRc < 0)
		throw CZorgeAssert(GetError());

	Out.SetSizeData(nRc);
}

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void CCoderRSA::Decrypt(const CMemBuffer& In, CMemBuffer& Out)
{
	if(!m_pRSA)
		throw CZorgeError("RSA not initialized.");

	CMutexGuard G(m_Lock);
	int nBlockSize = g_RSA_size(m_pRSA);
	if(nBlockSize < 1)
		throw CZorgeAssert(GetError());

	int nInputLen = In.GetSizeData();
	if(nInputLen > nBlockSize)
		throw CZorgeAssert("Input is too long.");

	unsigned char* pEncrypted = In.Get();

	unsigned char* pDecrypted = Out.Allocate(nBlockSize);
	int nRc = g_RSA_private_decrypt(nInputLen, pEncrypted, pDecrypted, m_pRSA, RSA_PKCS1_PADDING);
	if(nRc < 0)
		throw CZorgeAssert(GetError());
	Out.SetSizeData(nRc);
}

/*
#include <iostream>
struct C1
{
	C1()
	{
		try
		{
			CZorgeString s1, s2;
			CCoderRSA c1, c2;
			CMemBuffer b1, b2;

			c1.Init();

			if(0)
			{
				printf("public key\n%s\n", (const char*)c1.GetPublicKey());

				puts("Enter encrypted string:");
				CZorgeString strEncr;
				string s;
				getline(cin, s);
				strEncr = s.c_str();
				printf("\n[%s]\n", (const char*)strEncr);

				CMemBuffer Buf;
				if(Buf.InitFromHexChars(strEncr) != 0)
				{
					puts("Invalid input for bin");
					return;
				}
				Buf.Print();

				c1.Decrypt(Buf, b2);
				b2.ToString(s2);
				printf("Decrypted string = [%s]\n", (const char*)s2);
	return;
			}

			for(int i = 0; i < 1000; ++i)
			{
				const char* szInput = (const char*)s1;
				int nInputLen = s1.GetLength();
				printf("i=%d, len=%d s=%s\n", i, nInputLen, (const char*)s1);
				c1.Encrypt((unsigned char*)szInput, nInputLen, b1);

				printf("encrypted size = %ld\n\n", b1.GetSizeData());
				c1.Decrypt(b1, b2);

				b2.ToString(s2);
				printf("s2 = %s\n", (const char*)s2);
				s1 += "A";
			}
			puts("---------------> OK");
		}
		catch(ALL_ERRORS& e)
		{
			printf("Error = %s\n", e.what());
		}
	}
} aa1;
*/
