#include "CoderAESPool.h"
#include "ZorgeError.h"
#include <atomic>

CCoderAESPool g_RepoCoderPool;

std::atomic<unsigned int> g_CCoderPoolItem = 0;
std::atomic<unsigned int> g_CCoderPoolItem_max = 0;

void DiagCCoderPoolItem(CZorgeString& strOut)
{
	CZorgeString str;
	str.Format("CCoderPoolItem. Cur=%d, Max=%d\n", (unsigned int)g_CCoderPoolItem, (unsigned int)g_CCoderPoolItem_max);
	strOut += str;
}

struct CCoderPoolItemCounter
{
	~CCoderPoolItemCounter()
	{
		printf("g_CCoderPoolItem = %d (%d)\n", (unsigned int)g_CCoderPoolItem, (unsigned int)g_CCoderPoolItem_max);
	}
	static void Add()
	{
		g_CCoderPoolItem++;
		g_CCoderPoolItem_max++;
	}
} g_CCoderPoolItemCounter;

CCoderPoolItem::CCoderPoolItem(CCoderAES* pItem)
{
	CCoderPoolItemCounter::Add();
	if(pItem == nullptr)
		throw CZorgeAssert("Assert(pItem == nullptr)");

	m_pItem = pItem;
	m_bLocked = true;
	m_bFreeOnRelease = false;
}

void CCoderPoolItem::Release()
{
	//puts("CCoderPoolItem::Release()");
	if(m_bFreeOnRelease)
	{
		delete m_pItem, m_pItem = nullptr;
		m_bFreeOnRelease = false;
	}
	m_bLocked = false;
}

CCoderPoolItem::~CCoderPoolItem()
{
	g_CCoderPoolItem--;
//	printf("~CCoderPoolItem. m_bLocked=%d, m_bFreeOnRelease=%d, m_pItem=%X\n", m_bLocked, m_bFreeOnRelease, m_pItem);
	if(m_pItem)
		delete m_pItem;
}

void CCoderPoolItem::Print()
{
	printf("CCoderPoolItem. m_bLocked=%d, m_bFreeOnRelease=%d\n", m_bLocked, m_bFreeOnRelease);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
CCoderAESPool::CCoderAESPool()
{
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
CCoderAESPool::~CCoderAESPool()
{
	TCoders::iterator it = m_vCoders.begin();
	for(; it != m_vCoders.end(); ++it)
		delete *it;
	m_vCoders.clear();
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CCoderAESPool::Clear()
{
	CMutexGuard G(m_Lock);

	TCoders::iterator it = m_vCoders.begin();
	for(; it != m_vCoders.end(); ++it)
		delete *it;
	m_vCoders.clear();
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CCoderAESPool::Print()
{
	CMutexGuard G(m_Lock);
	printf("CCoderAESPool: max() = %ld, size() = %ld\n", m_nMaxSize, m_vCoders.size());
	for(CCoderPoolItem* p : m_vCoders)
	{
		printf("\t");
		p->Print();
	}
	puts("--> end of CCoderAESPool");
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CCoderAESPool::Set(size_t nMax, const CZorgeString& strPsw)
{
	if(nMax == m_nMaxSize && m_strPsw == strPsw)
		return;

	CMutexGuard G(m_Lock);
	if(m_vCoders.size() != 0)
		Clear();

	if(nMax < CODER_AES_POLL_MIN)
	{
		printf("Forcing coder pool size to minimal value %d\n", CODER_AES_POLL_MIN);
		nMax = CODER_AES_POLL_MIN;
	}

	m_nMaxSize = nMax;
	m_strPsw = strPsw;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
CCoderPoolItem*	CCoderAESPool::Get()
{
	CMutexGuard G(m_Lock);

	if(m_strPsw.IsEmpty())
		throw CZorgeAssert("m_strPsw.IsEmpty()");

	TCoders::iterator it = m_vCoders.begin();
	for(; it != m_vCoders.end(); ++it)
	{
		if((*it)->m_bLocked == false)
		{
			(*it)->m_bLocked = true;
			return *it;
		}
	}

	// No available coders. Create new one.
	CCoderAES* pCoder = new CCoderAES();
	pCoder->Init(m_strPsw);

	CCoderPoolItem* pItem = new CCoderPoolItem(pCoder);
	if(m_vCoders.size() < m_nMaxSize)
		m_vCoders.push_back(pItem);
	else
		pItem->m_bFreeOnRelease = true;

	return pItem;
}

/*
void F1()
{
	CCoderAESPool Pool;
	Pool.Set(2, "test");
	Pool.Print();
	Pool.Clear();
	puts("");

	//for(int i = 0; i < 10; ++i)
	{
		CCoderPoolItem* p = Pool.Get();
		CCoderPoolGuard G1(p);

		puts("-- before leaving scope");
		Pool.Print();
	}

	Pool.Set(2, "test1");

	puts("-- after leaving scope");
	Pool.Print();
}

struct C12
{
	C12()
	{
		F1();
		puts("-----------------------------------------> end.");
	}
} g12;
*/
