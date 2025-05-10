#include "Storage.h"
#include "SysUtils.h"
#include "ZorgeTimer.h"

extern CStorage* g_pStorageTest;// = 0;

static CZorgeTimer g_Timer;
static char g_szTimer[64];

#define IF_ERROR(s)	if(bError) { CZorgeString s1; s1.Format("%s, line %d", (const char*)s, __LINE__); throw CZorgeError(s1); }

static void CheckError(const char* szRef, const CZorgeError& e, const char* szMsg, int nLine)
{
	CZorgeString strErr(e.what());
	if(strErr.Find(szMsg) == STR_NOT_FOUND)
	{
		CZorgeString strE;
		strE.Format("[%s] Not found '%s' in '%s'. Line %d", szRef, szMsg, (const char*)strErr, nLine);
		throw CZorgeError(strE);
	}
}

static void Init(CZorgeString& strOut, int nMax)
{
	strOut = "";
	for(int i = 0; i < nMax + 1; ++i)
		strOut += "0";
}

void TestCreateNumbers_NoDevices_Invalid()
{
	printf("--- TestCreateNumbers_NoDevices_Invalid ");
	g_Timer.Start();
	CZorgeString strNum, strPin, strNewNum;

	// 1. Create same number twice.
	strNum = "101"; strPin = strNum;

	if(g_pStorageTest->FindNumber(strNum))
		g_pStorageTest->DeleteNumber(strNum, strPin);

	g_pStorageTest->AddNumber(strNum, strPin, strNewNum);
	bool bError = false;
	try
	{
		g_pStorageTest->AddNumber(strNum, strPin, strNewNum);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestCreateNumbers_NoDevices_Invalid 1.", e, "Number already exist", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(1). TestCreateNumbers_NoDevices_Invalid");

	// 2. Provide an empty number
	strNum = "";
	try
	{
		g_pStorageTest->AddNumber(strNum, strPin, strNewNum);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestCreateNumbers_NoDevices_Invalid 2.", e, "too short", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(2). TestCreateNumbers_NoDevices_Invalid");

	// 3. Provide an empty pin
	strNum = "101"; strPin = "";
	try
	{
		g_pStorageTest->AddNumber(strNum, strPin, strNewNum);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestCreateNumbers_NoDevices_Invalid 3.", e, "too short", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(3). TestCreateNumbers_NoDevices_Invalid");

	// 4. Provide number too long
	Init(strNum, MAX_DIGITS_IN_NUMBER);
	strPin = "101";
	try
	{
		g_pStorageTest->AddNumber(strNum, strPin, strNewNum);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestCreateNumbers_NoDevices_Invalid 4.", e, "too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(4). TestCreateNumbers_NoDevices_Invalid");

	// 5. Provide pin too long
	strNum = "101";
	Init(strPin, MAX_DIGITS_IN_PIN);
	try
	{
		g_pStorageTest->AddNumber(strNum, strPin, strNewNum);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestCreateNumbers_NoDevices_Invalid 5.", e, "too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(5). TestCreateNumbers_NoDevices_Invalid");

	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestDeleteNumbers_Invalid()
{
	printf("--- TestDeleteNumbers_Invalid ");
	g_Timer.Start();

	CZorgeString strNum, strPin, strNewNum;
	// 1. Delete not existing number
	strNum = "101"; strPin = strNum;

	if(!g_pStorageTest->FindNumber(strNum))
		g_pStorageTest->AddNumber(strNum, strPin, strNewNum);

	bool bError = false;
	try
	{
		strNum = "202";
		if(g_pStorageTest->FindNumber("202"))
			g_pStorageTest->DeleteNumber("202", "202");

		g_pStorageTest->DeleteNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestDeleteNumbers_Invalid 1.", e, "Number does not exist", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(1). TestDeleteNumbers_Invalid");

	// 2. Number is too long
	strPin = "101";
	Init(strNum, MAX_DIGITS_IN_NUMBER);
	bError = false;
	try
	{
		g_pStorageTest->DeleteNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestDeleteNumbers_Invalid 2.", e, "Number is too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(2). TestDeleteNumbers_Invalid");

	// 3. Number is too short
	strNum = "1"; strPin = "101";
	bError = false;
	try
	{
		g_pStorageTest->DeleteNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestDeleteNumbers_Invalid 3.", e, "Number is too short", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(3). TestDeleteNumbers_Invalid");

	// 4. Number is empty
	strNum = ""; strPin = "101";
	bError = false;
	try
	{
		g_pStorageTest->DeleteNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestDeleteNumbers_Invalid 4.", e, "Number is too short", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(4). TestDeleteNumbers_Invalid");

	// 5. Pin is too long
	strNum = "101";
	Init(strPin, MAX_DIGITS_IN_PIN);
	bError = false;
	try
	{
		g_pStorageTest->DeleteNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestDeleteNumbers_Invalid 5.", e, "Pin is too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(5). TestDeleteNumbers_Invalid");

	// 6. Pin is too short
	strNum = "101";	strPin = "1";
	bError = false;
	try
	{
		g_pStorageTest->DeleteNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestDeleteNumbers_Invalid 6.", e, "Pin is too short", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(6). TestDeleteNumbers_Invalid");

	// 7. Pin is empty
	strNum = "101";	strPin = "";
	bError = false;
	try
	{
		g_pStorageTest->DeleteNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestDeleteNumbers_Invalid 7.", e, "Pin is too short", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(7). TestDeleteNumbers_Invalid");

	// 8. Pin is invalid.
	strNum = "101";	strPin = "102";
	bError = false;
	try
	{
		g_pStorageTest->DeleteNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestDeleteNumbers_Invalid 8.", e, "Invalid pin", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(8). TestDeleteNumbers_Invalid");

	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestOpenNumbers_Invalid()
{
	printf("--- TestOpenNumbers_Invalid ");
	g_Timer.Start();

	CZorgeString strNum, strPin, strNewNum;

	// 1. Wrong number
	strNum = "101"; strPin = strNum;

	if(g_pStorageTest->FindNumber(strNum))
		g_pStorageTest->DeleteNumber(strNum, strPin);

	g_pStorageTest->AddNumber(strNum, strPin, strNewNum);
	bool bError = false;
	try
	{
		strNum = "000";
		g_pStorageTest->OpenNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestOpenNumbers_Invalid 1.", e, "Number does not exist", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(1). TestOpenNumbers_Invalid");

	// 2. Number is too short
	strNum = "1";
	bError = false;
	try
	{
		g_pStorageTest->OpenNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestOpenNumbers_Invalid 2.", e, "too short", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(2). TestOpenNumbers_Invalid");

	// 3. Number is too long
	Init(strNum, MAX_DIGITS_IN_NUMBER);
	bError = false;
	try
	{
		g_pStorageTest->OpenNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestOpenNumbers_Invalid 3.", e, "too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(3). TestOpenNumbers_Invalid");

	// 7. Wrong pin
	strNum = "101";
	strPin = "1212";
	bError = false;
	try
	{
		g_pStorageTest->OpenNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestOpenNumbers_Invalid 7.", e, "Invalid pin", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(7). TestOpenNumbers_Invalid");

	// 8. Pin too short
	strNum = "101"; strPin = "";
	bError = false;
	try
	{
		g_pStorageTest->OpenNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestOpenNumbers_Invalid 8.", e, "Pin is too short", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(8). TestOpenNumbers_Invalid");

	// 9. Pin too long
	strNum = "101";
	Init(strPin, MAX_DIGITS_IN_PIN);
	bError = false;
	try
	{
		g_pStorageTest->OpenNumber(strNum, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestOpenNumbers_Invalid 9.", e, "Pin is too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(9). TestOpenNumbers_Invalid");

	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestMessageSend_Invalid()
{
	printf("--- TestMessageSend_Invalid ");
	g_Timer.Start();

	CZorgeString strNumFrom("101"), strPinFrom("101"), strNewNum;
	CZorgeString strMsgId, strMsg, strNumTo;

	if(g_pStorageTest->FindNumber(strNumFrom))
		g_pStorageTest->DeleteNumber(strNumFrom, strPinFrom);
	g_pStorageTest->AddNumber(strNumFrom, strPinFrom, strNewNum);

	CNumDev Sender = { strNumFrom, strPinFrom };

	// 1. From number does not exist
	bool bError = false;
	try
	{
		Sender.m_strNum = "000"; strNumTo = "102";
		g_pStorageTest->SendMsg(Sender, strNumTo, strMsg, strMsgId);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageSend_Invalid 1.", e, "Number 'from' does not exist", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(1). TestMessageSend_Invalid");

	// 2. From number is too long
	Init(Sender.m_strNum, MAX_DIGITS_IN_NUMBER);
	bError = false;
	try
	{
		strNumTo = "102";
		g_pStorageTest->SendMsg(Sender, strNumTo, strMsg, strMsgId);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageSend_Invalid 2.", e, "Number is too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(2). TestMessageSend_Invalid");

	// 3. From number is too short
	bError = false;
	try
	{
		Sender.m_strNum = "1"; strNumTo = "102";
		g_pStorageTest->SendMsg(Sender, strNumTo, strMsg, strMsgId);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageSend_Invalid 3.", e, "Number is too short", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(3). TestMessageSend_Invalid");

	// 7. From pin is invalid
	bError = false;
	try
	{
		Sender.m_strPin = "000";
		Sender.m_strNum = "101"; strNumTo = "101";
		g_pStorageTest->SendMsg(Sender, strNumTo, strMsg, strMsgId);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageSend_Invalid 7.", e, "Invalid pin", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(7). TestMessageSend_Invalid");

	// 8. From pin is too short
	bError = false;
	try
	{
		Sender.m_strPin = "0";
		Sender.m_strNum = "101"; strNumTo = "101";
		g_pStorageTest->SendMsg(Sender, strNumTo, strMsg, strMsgId);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageSend_Invalid 8.", e, "Pin is too short", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(8). TestMessageSend_Invalid");

	// 9. From pin is too long
	bError = false;
	try
	{
		Init(Sender.m_strPin, MAX_DIGITS_IN_PIN);
		Sender.m_strNum = "101"; strNumTo = "101";
		g_pStorageTest->SendMsg(Sender, strNumTo, strMsg, strMsgId);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageSend_Invalid 9.", e, "Pin is too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(9). TestMessageSend_Invalid");

	// 10. To number not exist
	bError = false;
	try
	{
		Sender.m_strPin = "101"; Sender.m_strNum = "101";
		strNumTo = "000";
		g_pStorageTest->SendMsg(Sender, strNumTo, strMsg, strMsgId);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageSend_Invalid 10.", e, "Number 'to' does not exist", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(10). TestMessageSend_Invalid");

	// 11. To number is too short
	bError = false;
	try
	{
		Sender.m_strPin = "101"; Sender.m_strNum = "101";
		strNumTo = "0";
		g_pStorageTest->SendMsg(Sender, strNumTo, strMsg, strMsgId);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageSend_Invalid 11.", e, "Number is too short", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(11). TestMessageSend_Invalid");

	// 12. To number is too long
	bError = false;
	try
	{
		Sender.m_strPin = "101"; Sender.m_strNum = "101";
		Init(strNumTo, MAX_DIGITS_IN_NUMBER);
		g_pStorageTest->SendMsg(Sender, strNumTo, strMsg, strMsgId);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageSend_Invalid 12.", e, "Number is too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(12). TestMessageSend_Invalid");

	// 10. Msg is too long
	bError = false;
	try
	{
		Init(strMsg, MAX_MSG_LEN_BYTES + 1);
		Sender.m_strPin = "101"; Sender.m_strNum = "101";
		strNumTo = "101";
		g_pStorageTest->SendMsg(Sender, strNumTo, strMsg, strMsgId);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageSend_Invalid 13.", e, "Message is too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(13). TestMessageSend_Invalid");

	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestMessageRead_Invalid()
{
	printf("--- TestMessageRead_Invalid ");
	g_Timer.Start();

	CZorgeString strNumFrom("101"), strPinFrom("101"), strNewNum;
	CZorgeString strMsgId;
	CMessage* pMsg = 0;

	if(g_pStorageTest->FindNumber(strNumFrom))
		g_pStorageTest->DeleteNumber(strNumFrom, strPinFrom);
	g_pStorageTest->AddNumber(strNumFrom, strPinFrom, strNewNum);

	CNumDev From = { strNumFrom, strPinFrom };

	// 1. Invalid from number
	bool bError = false;
	try
	{
		From.m_strPin = "101"; From.m_strNum = "000";
		pMsg = g_pStorageTest->ReadMessage(From, strMsgId);
		delete pMsg;
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageRead_Invalid 1.", e, "Number 'from' does not exist", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(1). TestMessageRead_Invalid");

	// 2. Number is too long
	bError = false;
	try
	{
		Init(From.m_strNum, MAX_DIGITS_IN_NUMBER);
		From.m_strPin = "101";
		pMsg = g_pStorageTest->ReadMessage(From, strMsgId);
		delete pMsg;
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageRead_Invalid 2.", e, "Number is too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(2). TestMessageRead_Invalid");

	// 3. Number is too short
	bError = false;
	try
	{
		From.m_strNum = "1";
		From.m_strPin = "101";
		pMsg = g_pStorageTest->ReadMessage(From, strMsgId);
		delete pMsg;
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageRead_Invalid 3.", e, "Number is too short", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(3). TestMessageRead_Invalid");

	// 4. Pin is invalid
	bError = false;
	try
	{
		From.m_strNum = "101";
		From.m_strPin = "000";
		pMsg = g_pStorageTest->ReadMessage(From, strMsgId);
		delete pMsg;
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageRead_Invalid 4.", e, "Invalid pin", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(4). TestMessageRead_Invalid");

	// 5. Pin is too long
	bError = false;
	try
	{
		Init(From.m_strPin, MAX_DIGITS_IN_PIN);
		From.m_strNum = "101";
		pMsg = g_pStorageTest->ReadMessage(From, strMsgId);
		delete pMsg;
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageRead_Invalid 5.", e, "Pin is too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(5). TestMessageRead_Invalid");

	// 6. Pin is too short
	bError = false;
	try
	{
		From.m_strPin = "1";
		From.m_strNum = "101";
		pMsg = g_pStorageTest->ReadMessage(From, strMsgId);
		delete pMsg;
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageRead_Invalid 6.", e, "Pin is too short", __LINE__);
		bError = false;
	}

	IF_ERROR("Expected an error(6). TestMessageRead_Invalid");

	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestMessageDelete_Invalid()
{
	printf("--- TestMessageDelete_Invalid ");
	g_Timer.Start();

	CZorgeString strNumFrom("101"), strPinFrom("101"), strNewNum;
	CZorgeString strMsgId;

	if(g_pStorageTest->FindNumber(strNumFrom))
		g_pStorageTest->DeleteNumber(strNumFrom, strPinFrom);
	g_pStorageTest->AddNumber(strNumFrom, strPinFrom, strNewNum);

	CNumDev From = { strNumFrom, strPinFrom };

	// 1. Invalid from number
	bool bError = false;
	try
	{
		From.m_strNum = "000"; strMsgId = "12121212";
		g_pStorageTest->DeleteMessage(From, strMsgId);
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageDelete_Invalid 1.", e, "Number 'from' does not exist", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(1). TestMessageDelete_Invalid");

	// 2. Number is too long
	bError = false;
	try
	{
		Init(From.m_strNum, MAX_DIGITS_IN_NUMBER);
		g_pStorageTest->DeleteMessage(From, strMsgId);
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageDelete_Invalid 2.", e, "Number is too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(2). TestMessageDelete_Invalid");

	// 3. Number is too short
	bError = false;
	try
	{
		From.m_strNum = "";
		g_pStorageTest->DeleteMessage(From, strMsgId);
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageDelete_Invalid 3.", e, "Number is too short", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(3). TestMessageDelete_Invalid");

	// 7. Invalid pin
	bError = false;
	try
	{
		From.m_strNum = strNumFrom;
		From.m_strPin = "9999999";
		g_pStorageTest->DeleteMessage(From, strMsgId);
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageDelete_Invalid 7.", e, "Invalid pin", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(7). TestMessageDelete_Invalid");

	// 8. Pin is too long
	bError = false;
	try
	{
		Init(From.m_strPin, MAX_DIGITS_IN_PIN);
		g_pStorageTest->DeleteMessage(From, strMsgId);
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageDelete_Invalid 8.", e, "Pin is too long.", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(8). TestMessageDelete_Invalid");

	// 9. Pin is too short
	bError = false;
	try
	{
		From.m_strPin = "";
		g_pStorageTest->DeleteMessage(From, strMsgId);
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageDelete_Invalid 9.", e, "Pin is too short.", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(9). TestMessageDelete_Invalid");

	// 10. Invalid msg id
	bError = false;
	try
	{
		From.m_strPin = "101";
		strMsgId = "not existing id";
		g_pStorageTest->DeleteMessage(From, strMsgId);
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageDelete_Invalid 10.", e, "Message not found", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(10). TestMessageDelete_Invalid");

	// 11. Empty msg id
	bError = false;
	try
	{
		From.m_strPin = "101";
		strMsgId = "";
		g_pStorageTest->DeleteMessage(From, strMsgId);
	}
	catch(CZorgeError& e)
	{
		CheckError("TestMessageDelete_Invalid 11.", e, "Message id is empty.", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(11). TestMessageDelete_Invalid");

	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}

void TestChangePin_Invalid()
{
	printf("--- TestChangePin_Invalid ");
	g_Timer.Start();

	CZorgeString strNum("101"), strPin("101"), strNewNum, strNewPin;

	if(g_pStorageTest->FindNumber(strNum))
		g_pStorageTest->DeleteNumber(strNum, strPin);
	g_pStorageTest->AddNumber(strNum, strPin, strNewNum);

	CNumDev From = { strNum, strPin };

	// 1. Invalid number
	bool bError = false;
	try
	{
		From.m_strNum = "000";
		g_pStorageTest->ChangePin(From, strPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestChangePin_Invalid 1.", e, "Number does not exist", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(1). TestChangePin_Invalid");

	// 2. Pin is invalid
	bError = false;
	try
	{
		From.m_strNum = "101"; From.m_strPin = "invalid pin";
		strNewPin = "101";
		g_pStorageTest->ChangePin(From, strNewPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestChangePin_Invalid 2.", e, "Invalid pin", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(2). TestChangePin_Invalid");

	// 5. New pin is too long
	bError = false;
	try
	{
		From.m_strNum = "101"; From.m_strPin = "101";
		Init(strNewPin, MAX_DIGITS_IN_PIN);
		g_pStorageTest->ChangePin(From, strNewPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestChangePin_Invalid 3.", e, "Pin is too long", __LINE__);
		bError = false;
	}
	IF_ERROR("Expected an error(3). TestChangePin_Invalid");

	// 6. New pin is too short
	bError = false;
	try
	{
		From.m_strNum = "101"; From.m_strPin = "101";
		strNewPin = "1";
		g_pStorageTest->ChangePin(From, strNewPin);
		bError = true;
	}
	catch(CZorgeError& e)
	{
		CheckError("TestChangePin_Invalid 4.", e, "Pin is too short", __LINE__);
		bError = false;
	}

	IF_ERROR("Expected an error(4). TestChangePin_Invalid");

	printf("%s\n", g_Timer.ToString(g_szTimer, 64));
}
